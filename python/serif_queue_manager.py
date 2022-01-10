#!/bin/env python
#
# Copyright 2013 by BBN Technologies Corp.
# All Rights Reserved.

import os, sys, re, subprocess, shutil, collections, time, errno
import signal, optparse, math, traceback

######################################################################
# CONFIGURATION
######################################################################

DEFAULT_PIPELINE = [
    ('values', 1),
    ('parse', 4),
    ('output', 2),
    ]

ICEWS_CLI=True

######################################################################
# Special queues
######################################################################

PIPELINE_START = 'start'
ICEWS_FEEDER = 'icews-feeder'

######################################################################
# Directory Layout
######################################################################
#  rootdir/
#    worker-<N>/                 # <N> is a worker-id
#      worker.par
#      queues
#      pid
#      times
#    queue-<NAME>                # <NAME> is usually a stage name
#      file.ready
#      file.<N>.working          # <N> is a worker-id
#      file.<N>.writing.xml
#      file.failed

######################################################################
# Worker
######################################################################
WORKING_EXTENSION = ".working"
WRITING_EXTENSION = ".writing.xml"
READY_EXTENSION = ".ready"
FAILED_EXTENSION = ".failed"
DONE_FILENAME = "done"
GAVE_UP_EXTENSION = ".failed_twice"

class Worker(object):
    """
    A wrapper for a serif disk-queue worker,
    """
    QUIET = False
    SOURCE_FORMAT = 'sgm'

    def __init__(self, rootdir, worker_id):
        self._rootdir = rootdir
        self._worker_id = worker_id
        self._worker_dir = self._get_worker_dir(rootdir, worker_id)
        assert os.path.isdir(self._worker_dir)
        self._get_pid()
        self._get_queues()
        self._get_times()
        
    worker_id = property(lambda self:self._worker_id)
    src_queue = property(lambda self:self._src_queue)
    dst_queue = property(lambda self:self._dst_queue)
    work_time = property(lambda self:self._work_time)
    wait_time = property(lambda self:self._wait_time)
    block_time = property(lambda self:self._block_time)
    overhead_time = property(lambda self:self._overhead_time)
    total_time = property(
        lambda self:self._work_time+self._wait_time+
        self._block_time+self._overhead_time)
    work_pct = property(lambda self:self._work_time/(self.total_time or 1))
    wait_pct = property(lambda self:self._wait_time/(self.total_time or 1))
    block_pct = property(lambda self:self._block_time/(self.total_time or 1))
    overhead_pct = property(lambda self:self._overhead_time/(self.total_time or 1))
    num_docs = property(lambda self:self._num_docs)
    pid = property(lambda self:self._pid)
    
    #/////////////////////////////////////////////////////////////////
    # Instance Methods
    #/////////////////////////////////////////////////////////////////

    def is_alive(self):
        """
        Return true if this worker is still running.
        """
        if self._pid not in (None, 0):
            if not pid_exists(self._pid):
                print 'Worker %s (pid=%s) exited.' % (
                    self._worker_id, self._pid)
                self._pid = None
        return self._pid is not None

    def close(self, block=False):
        quitpath = self._get_quit_path(self._rootdir, self._worker_id)
        open(quitpath, 'wb').close()
        if block:
            while self.is_alive():
                time.sleep(1)

    def kill(self):
        """
        Stop this worker's SERIF process (if it is running).  This
        does *not* delete/clean up the worker's directory.  This kills
        the worker immediately, and may cause stale file locks.
        """
        self.is_alive()
        if self._pid is not None:
            os.kill(self._pid, signal.SIGKILL)
            self._pid = None
        pid_path = self._get_pid_path(self._rootdir, self._worker_id)
        if os.path.exists(pid_path):
            os.remove(pid_path)

    def cleanup(self):
        if self._worker_dir is not None:
            self.kill() # make sure the process is dead.
            try: shutil.rmtree(self._worker_dir)
            except OSError:
                time.sleep(1)
                try: shutil.rmtree(self._worker_dir)
                except OSError: pass
            self._worker_dir = None
            self._worker_id = None
            self._pid = None

    def _get_pid(self):
        self._pid = None
        pidfile = self._get_pid_path(self._rootdir, self._worker_id)
        pidstr = read_with_retry(pidfile)
        if pidstr:
            self._pid = int(pidstr)

    def _get_queues(self):
        self._src_queue = self._dst_queue = None
        qfile = self._get_queues_path(self._rootdir, self._worker_id)
        queues = read_with_retry(qfile)
        m = re.search('src_queue:\s*(\S+)', queues)
        if m: self._src_queue = m.group(1)
        m = re.search('dst_queue:\s*(\S+)', queues)
        if m: self._dst_queue = m.group(1)

    def _get_times(self):
        self._work_time = self._block_time = 0
        self._wait_time = self._overhead_time = 0
        self._num_docs = 0
        timesfile = self._get_timer_path(self._rootdir, self._worker_id)
        times = read_with_retry(timesfile, retries=1)
        m = re.match(r'Work\t(.*)\nWait\t(.*)\nBlock\t(.*)'
                     r'\nOverhead\t(.*)\nDocs\t(.*)\n',
                     times)
        if m is None: return
        work, wait, block, overhead, docs = m.groups()
        self._work_time = float(work)
        self._wait_time = float(wait)
        self._block_time = float(block)
        self._overhead_time = float(overhead)
        self._num_docs = int(docs or 0)

    #/////////////////////////////////////////////////////////////////
    # Factory Methods
    #/////////////////////////////////////////////////////////////////

    @classmethod
    def start(cls, rootdir, master_par, src_queue, dst_queue, is_final):
        # Create a new directory for the worker to use.
        worker_id = cls._make_worker_dir(rootdir, src_queue, dst_queue)
        # Create the worker's SERIF parameter file
        cls._make_worker_par(rootdir, worker_id, master_par,
                             src_queue, dst_queue, is_final)
        # Save info about our input/output queues.
        sout = open(cls._get_queues_path(rootdir, worker_id), 'wb')
        sout.write('src_queue: %s\ndst_queue: %s\n' %
                   (src_queue, dst_queue))
        sout.close()
        # Make sure src/dst directories exist
        if not os.path.exists(cls.get_queue_dir(rootdir, src_queue)):
            os.makedirs(cls.get_queue_dir(rootdir, src_queue))
        if not os.path.exists(cls.get_queue_dir(rootdir, dst_queue)):
            os.makedirs(cls.get_queue_dir(rootdir, dst_queue))
        # Run Serif
        cls._start_serif_subprocess(rootdir, worker_id)
        # Return the worker
        return cls(rootdir, worker_id)

    @classmethod
    def get_all(cls, rootdir):
        workers = []
        if not os.path.isdir(rootdir): return workers
        for d in os.listdir(rootdir):
            if not os.path.isdir(os.path.join(rootdir, d)):
                continue
            m = re.match(r'^worker-(.*)$', d)
            if m:
                worker_id = m.group(1)
                workers.append(cls(rootdir, worker_id))
        workers.sort(key=lambda w:int(w.worker_id))
        return workers

    @classmethod
    def has_timer_path(cls, rootdir, worker_id):
        return os.path.exists(cls._get_timer_path(rootdir, worker_id))

    #/////////////////////////////////////////////////////////////////
    # Class Methods & Static Methods
    #/////////////////////////////////////////////////////////////////

    @classmethod
    def _make_worker_dir(cls, rootdir, src_queue, dst_queue):
        max_worker_id = 0
        for subdir in os.listdir(rootdir):
            m = re.match(r'worker-(\d+)', subdir)
            if m:
                max_worker_id = max(max_worker_id, int(m.group(1)))
        worker_id = '%s' % (max_worker_id+1)
        os.mkdir(cls._get_worker_dir(rootdir, worker_id))
        return worker_id

    @classmethod
    def _start_serif_subprocess(cls, rootdir, worker_id):
        # Run Serif
        par_path = cls._get_par_path(rootdir, worker_id)
        cmd = [cls.find_serif_bin(), par_path]
        #if cls.QUIET: cmd.append('-q')
        #print cmd
        pidfile = cls._get_pid_path(rootdir, worker_id)
        if cls.QUIET:
            outfile = os.devnull
        else:
            outfile = cls._get_worker_dir(rootdir, worker_id, 'out.txt')
        #print 'Writing worker %s output to %s' % (worker_id, outfile)
        start_daemon(cmd, pidfile, open(outfile, 'wb'))

    @classmethod
    def _make_worker_par(cls, rootdir, worker_id, master_par,
                         src_queue, dst_queue, is_final):
        # Decide what our input/output directories are
        src_dir=cls.get_queue_dir(rootdir, src_queue)
        dst_dir=cls.get_queue_dir(rootdir, dst_queue)
        expt_dir=cls._get_worker_dir(rootdir, worker_id)

        # Defaults:
        queue_driver = 'disk'
        source_format = 'serifxml'
        output_format = 'serifxml'
        start_stage = '%s+1' % src_queue
        end_stage = '%s' % dst_queue
        max_dst_files = 500

        # The final directory has no size limit.
        if is_final:
            max_dst_files = 0

        # The start stage expects sgm input by default.
        if src_queue.lower() == 'start':
            source_format = cls.SOURCE_FORMAT

        # Special handling for ICEWS feeder queue.
        if dst_queue == ICEWS_FEEDER:
            end_stage = 'start'
            queue_driver = 'icews-disk-feeder'
        if src_queue == ICEWS_FEEDER:
            start_stage = 'start'

        # If the final queue is icews-output, then don't actually save
        # the xml -- just write to the database.
        if is_final and dst_queue == 'icews-output':
            output_format = 'NONE'

        # Assemble the parameter file
        param_file = cls.PARAM_FILE % dict(
            start_stage=start_stage,
            end_stage=end_stage,
            src_dir=src_dir,
            dst_dir=dst_dir,
            source_format=source_format,
            output_format=output_format,
            expt_dir=expt_dir,
            serif_master_par=master_par,
            timer_file=cls._get_timer_path(rootdir, worker_id),
            quit_file=cls._get_quit_path(rootdir, worker_id),
            max_dst_files=max_dst_files,
            queue_driver=queue_driver,
            worker_ext='.%s' % worker_id)
        # Write the parameter file
        out = open(cls._get_par_path(rootdir, worker_id), 'wb')
        out.write(param_file)
        out.close()

    @classmethod
    def get_queue_dir(cls, rootdir, stage):
        return os.path.join(rootdir, 'queue-%s' % stage)

    @classmethod
    def _get_worker_dir(cls, rootdir, worker_id, extra=None):
        d = os.path.join(rootdir, 'worker-%s' % worker_id)
        if extra: d = os.path.join(d, extra)
        return d

    @classmethod
    def _get_par_path(cls, rootdir, worker_id):
        return cls._get_worker_dir(rootdir, worker_id, 'worker.par')

    @classmethod
    def _get_pid_path(cls, rootdir, worker_id):
        return cls._get_worker_dir(rootdir, worker_id, 'pid')

    @classmethod
    def _get_timer_path(cls, rootdir, worker_id):
        return cls._get_worker_dir(rootdir, worker_id, 'times')

    @classmethod
    def _get_quit_path(cls, rootdir, worker_id):
        return cls._get_worker_dir(rootdir, worker_id, 'quit')

    @classmethod
    def _get_queues_path(cls, rootdir, worker_id):
        return cls._get_worker_dir(rootdir, worker_id, 'queues')

    _SERIF_BIN = None
    _SERIF_BIN_PATH = [
        '../bin/x86_64',
        '../../../Core/SERIF/build/bin',
        ]

    @classmethod
    def set_serif_bin(cls, path):
        assert os.path.exists(path)
        cls._SERIF_BIN = path

    @classmethod
    def find_serif_bin(cls):
        if cls._SERIF_BIN is None:
            here = os.path.split(__file__)[0]
            for path in cls._SERIF_BIN_PATH:
                binpath = os.path.join(here, path, 'Serif')
                if os.path.exists(binpath):
                    #print 'Found Serif: %r' % binpath
                    cls._SERIF_BIN = binpath
                    break
            else:
                raise ValueError('Unable to find Serif binary')
        return cls._SERIF_BIN

    PARAM_FILE = '''
    #experiment_dir:     %(expt_dir)s
    UNSET experiment_dir
    UNSET batch_file

    queue_driver:                 %(queue_driver)s
    disk_queue_src:               %(src_dir)s
    disk_queue_dst:               %(dst_dir)s
    disk_queue_timer_file:        %(timer_file)s
    disk_queue_quit_file:         %(quit_file)s
    disk_queue_max_dst_files:     %(max_dst_files)s
    disk_queue_worker_ext:        %(worker_ext)s

    INCLUDE %(serif_master_par)s

    OVERRIDE start_stage:         %(start_stage)s
    OVERRIDE end_stage:           %(end_stage)s

    OVERRIDE source_format:       %(source_format)s
    OVERRIDE input_type:          auto
    OVERRIDE output_format:       %(output_format)s
    OVERRIDE state_saver_stages:  NONE
    OVERRIDE use_lazy_model_loading: false
    OVERRIDE ignore_errors:          false
    '''

#icews_min_storyid: 0
#icews_max_storyid: 1000
#icews_min_story_ingest_date: YYYY-MM-DD
#icews_max_story_ingest_date: YYYY-MM-DD
#icews_story_source: XXXXX


######################################################################
# Monitor
######################################################################

class QueueMonitor(object):
    def __init__(self, rootdir, parfile, verbosity, pipeline):
        self.rootdir = rootdir
        self.parfile = parfile
        self.verbosity = verbosity
        self.pipeline = pipeline
        if not os.path.exists(rootdir):
            self.log('Creating disk queue root %r' % rootdir)
            os.makedirs(rootdir)
        self.workers = Worker.get_all(rootdir)
        self._tput_info = []
        self.max_total_mem = 0
        self.max_total_res_mem = 0

    def log(self, msg, min_verbosity=1, newline=True):
        if newline: msg = msg.rstrip()+'\n'
        if self.verbosity >= min_verbosity:
            try:
                sys.stdout.write(msg)
                sys.stdout.flush()
            except IOError:
                pass

    def check_abandoned_jobs(self):
        worker_ids = [w.worker_id for w in self.workers if w.is_alive()]
        for dir in os.listdir(self.rootdir):
            m = re.match('queue-(.*)', dir)
            if m:
                dirpath = os.path.join(self.rootdir, dir)
                for file in os.listdir(dirpath):
                    if file.endswith(WORKING_EXTENSION):
                        file_base = file[:-len(WORKING_EXTENSION)]
                        src, ext = os.path.splitext(file_base)
                        worker_id = ext[1:]
                        if worker_id not in worker_ids:
                            self.log('Worker %s abandoned %s' %
                                     (worker_id, src), 2)
                            workers_with_id = [w for w in self.workers if w.worker_id == worker_id]
                            # Move it back into the queue.
                            os.rename(os.path.join(dirpath, file),
                                      os.path.join(dirpath, src))
                        else:
                            self.log('Worker %s is working on %s' %
                                     (worker_id, src), 3)
                    if file.endswith(WRITING_EXTENSION):
                        file_base = file[:-len(WRITING_EXTENSION)]
                        src, ext = os.path.splitext(file_base)
                        worker_id = ext[1:]
                        if worker_id not in worker_ids:
                            self.log('Worker %s did not finish writing %s' %
                                     (worker_id, src), 2)
                            os.remove(os.path.join(dirpath, file))
                        else:
                            self.log('Worker %s is writing %s' %
                                     (worker_id, src), 3)

    def close_all(self, cleanup=True):
        if not self.workers:
            self.log('No workers found')
            return
        # Send a close signal to all workers.
        alive = [w for w in self.workers if w.is_alive()]
        self.log('Sending exit signal to %d workers...' % len(alive))
        for w in self.workers:
            w.close(block=False)
        # Wait for them all to finish working.
        self.log('Waiting for workers to quit...', newline=False)
        sys.stdout.flush()
        for w in self.workers:
            w.close(block=True)
            self.log('.', newline=False)
        if cleanup:
            for w in self.workers:
                w.cleanup()
        self.log('')

    def kill_all(self, cleanup=True):
        if  not self.workers:
            self.log('No workers found')
            return
        alive = [w for w in self.workers if w.is_alive()]
        self.log('Killing %d workers...' % len(alive))
        for w in self.workers:
            w.kill()
        if cleanup:
            for w in self.workers:
                w.cleanup()

    def _count_queue_files(self, stage):
        stagedir = os.path.join(self.rootdir, 'queue-%s' % stage)
        waiting = running = 0
        if not os.path.exists(stagedir): return 0,0
        for f in os.listdir(stagedir):
            if not os.path.isfile(os.path.join(stagedir,f)): continue
            if f.endswith(WRITING_EXTENSION):
                continue
            elif f.endswith(WORKING_EXTENSION):
                running += 1
            elif f.endswith(READY_EXTENSION):
                waiting += 1
            elif f.endswith(FAILED_EXTENSION):
                waiting += 1
        return waiting, running

    def check_memory(self):
        total_mem = 0
        total_res_mem = 0
        for worker in self.workers:
            if worker.is_alive():
                worker_info = get_process_info(worker.pid)
                total_mem += parse_memory(worker_info['VmSize'])
                total_res_mem += parse_memory(worker_info['VmRSS'])
        self.max_total_mem = max(self.max_total_mem, total_mem)
        self.max_total_res_mem = max(self.max_total_res_mem, total_res_mem)

    def list_workers(self, clear_scr=False):
        # Get a list of stage pairs.
        queues = [PIPELINE_START] + [p[0] for p in self.pipeline]
        queuepairs = [(queues[i-1], queues[i])
                      for i in range(1, len(queues))]

        # Sort the workers by src&dst.
        worker_dict = collections.defaultdict(list)
        for w in self.workers:
            key = w.src_queue, w.dst_queue
            worker_dict[key].append(w)
            #if w.src_queue is None or w.dst_queue is None: continue
            #if key not in queuepairs: queuepairs.append(key)

        total_mem = 0
        total_res_mem = 0

        TEMPLATE = ' '*9 + '%-10s %4s %8s %6s %6s %6s %6s %10s'
        lines = []
        #lines.append('Pipeline: %r' % self.rootdir)
        lines.append(' Docs  Stage         '+'Workers'.center(55,'_'))
        lines.append(TEMPLATE % ('', 'Id', 'Memory', 'Work', 'Wait',
                                 'Block', 'Ovrhd', 'Time/Doc'))
        lines.append('='*20+' '+'='*55)
        src = PIPELINE_START
        for stage in self.pipeline:
            dst = stage[0]
            queue_workers = worker_dict[src,dst]
            waiting, running = self._count_queue_files(src)
            if self.stage_is_done(src, dst):
                done = '(done)'
            else:
                done = ''
            lines.append('[%4d] %s %s' % (waiting, src, done))
            for i, worker in enumerate(queue_workers):
                if worker.is_alive():
                    worker_info = get_process_info(worker.pid)
                    memory = format_memory(worker_info['VmSize'])
                    total_mem += parse_memory(worker_info['VmSize'])
                    total_res_mem += parse_memory(worker_info['VmRSS'])
                else:
                    memory = '(exited)'
                if worker.num_docs:
                    tput = ((worker.work_time+worker.overhead_time) /
                            worker.num_docs)
                else:
                    tput = 0
                if i and i == len(queue_workers)-1:
                    arrow = 'V'
                else:
                    arrow = '|'
                lines.append(TEMPLATE % (arrow, worker.worker_id, memory,
                                         '%.1f%%' % (100.0*worker.work_pct),
                                         '%.1f%%' % (100.0*worker.wait_pct),
                                         '%.1f%%' % (100.0*worker.block_pct),
                                         '%.1f%%' % (100.0*worker.overhead_pct),
                                         format_time(tput)))
            if not queue_workers:
                lines.append(TEMPLATE % ('|', 'None', '-', '-',
                                         '-', '-', '-', '-'))
            if len(queue_workers) <= 1:
                lines.append(TEMPLATE % ('V', '', '', '', '', '', '', ''))
            src = dst

        self.max_total_mem = max(self.max_total_mem, total_mem)
        self.max_total_res_mem = max(self.max_total_res_mem, total_res_mem)

        # Final line for final dst queue
        waiting, running = self._count_queue_files(dst)
        lines.append('[%4d] %s' % (waiting, dst))

        docs_finished = sum(w.num_docs or 0 for w in self.workers if
                           w.dst_queue == self.pipeline[-1][0])
        lines.append('='*20+' '+'='*55)
        lines.append('%40s: %s' % ('Max total memory',
                                   format_memory(self.max_total_mem)))
        lines.append('%40s: %s' % ('Max resident memory',
                                   format_memory(self.max_total_res_mem)))
        lines.append('%40s: %d' % ('Documents processed', docs_finished))


        try:
            tput_line = self.get_throughput()
            if tput_line: lines.append(tput_line)
        except ValueError:
            pass

        if clear_scr:
            clear_screen()
        print '\n'.join(lines)
        sys.stdout.flush()

    def update_tputs(self):
        docs_finished = sum(w.num_docs or 0 for w in self.workers if
                            w.dst_queue == self.pipeline[-1][0])
        # Add new throughput info.
        now = time.time()
        if docs_finished:
            self._tput_info.append([now, docs_finished])

    def get_throughput(self):
        # Don't display tput info until we have a reasonable amount
        if len(self._tput_info) < 3: return
        # Calculate
        tputs = []
        dts = []
        stride = max(3, len(self._tput_info)/100)
        for i in range(stride, len(self._tput_info), stride):
            t0, c0 = self._tput_info[i-stride]
            t1, c1 = self._tput_info[i]
            dts.append(1000*(t1-t0))
            tputs.append(3600.0*(c1-c0)/(t1-t0))
        if not tputs:
            return
        avg = sum(tputs)/len(tputs)
        dev = math.sqrt(sum((x-avg)**2 for x in tputs)/len(tputs))
        avg_dt = sum(dts)/len(dts)
        out = '%40s: %d docs/hr' % ('Throughput', avg)
        out += ' (stddev=%d)' % dev
        #out += '\n%50s: %d' % ('StdDev', dev)
        #out += '\n%50s: %d-%d docs/hr' % ('Range', min(tputs), max(tputs))
        #out += '\n%42s: %s' % ('Window', format_time(1000*(now-self._tput_info[0][0])))
        #out += '\n%42s: %d' % ('Samples', len(tputs))
        return out

    def check_pipeline(self):
        changed = False
        all_done = True
        # Sort the workers by src&dst.
        worker_dict = collections.defaultdict(list)
        dead_worker_dict = collections.defaultdict(list)
        for w in self.workers:
            if w.is_alive():
                worker_dict[w.src_queue, w.dst_queue].append(w)
            else:
                dead_worker_dict[w.src_queue, w.dst_queue].append(w)
                #w.cleanup()

        # Check that we have the desired number of each type of
        # worker.
        closed_workers = []
        src_queue = PIPELINE_START
        for dst_queue, num_workers in self.pipeline:
            done = self.stage_is_done(src_queue, dst_queue)
            if done:
                num_workers = 0
            elif num_workers > 0:
                all_done = False

            queue_workers = worker_dict[src_queue, dst_queue]
            dead_queue_workers = dead_worker_dict[src_queue, dst_queue]

            if (len(dead_queue_workers) > 0 and 
                self.check_for_startup_error(dead_queue_workers, src_queue, dst_queue)):
                raise SerifStartupError('Serif could not start up between queues: ' +
                                        str(src_queue) + ' ' + str(dst_queue))

            for i in range(len(queue_workers), num_workers):
                self.log('Starting new worker:   %20s -> %s' %
                         (src_queue, dst_queue), 0)
                is_final = (dst_queue==self.pipeline[-1][0])
                self.workers.append(
                    Worker.start(self.rootdir, self.parfile,
                                 src_queue, dst_queue, is_final))
                changed = True
            for i in range(num_workers, len(queue_workers)):
                self.log('Stopping extra worker: %20s -> %s' %
                         (src_queue, dst_queue))
                queue_workers[i].close(block=False)
                closed_workers.append(queue_workers[i])
            del queue_workers[:]
            src_queue = dst_queue

        for (src, dst), queue_workers in worker_dict.items():
            for worker in queue_workers:
                self.log('Stopping extra worker: %20s -> %s' %
                         (src, dst))
                worker.close(block=False)
                closed_workers.append(worker)

        if closed_workers:
            changed = True
            self.log('Waiting for closed workers to exit...')
            for worker in closed_workers:
                worker.close(block=True)
                #self.workers.remove(worker)

        #if changed:
        #    del self._tput_info[:] # clear througput info.

        # Wait a moment for all new subprocesses to start.
        if changed:
            time.sleep(2)
        return changed, all_done

    def stage_is_done(self, src_queue, dst_queue):
        for queue in src_queue, dst_queue:
            queue_dir = Worker.get_queue_dir(self.rootdir, queue)
            if not os.path.exists(queue_dir):
                return False
            done_filename = os.path.join(queue_dir, DONE_FILENAME)
            if not os.path.exists(done_filename):
                return False

        src_queue_dir = Worker.get_queue_dir(self.rootdir, src_queue)
        for f in os.listdir(src_queue_dir):
            if (f.endswith(WRITING_EXTENSION) or
                f.endswith(WORKING_EXTENSION) or
                f.endswith(READY_EXTENSION) or
                f.endswith(FAILED_EXTENSION)):
                return False
        return True

    def check_for_startup_error(self, dead_queue_workers, src_queue, dst_queue):
        # Look for evidence of startup and return False if found
        for w in dead_queue_workers:
            if Worker.has_timer_path(self.rootdir, w.worker_id):
                return False
        return True
    
    def show_failures(self):
        print
        for dir in os.listdir(self.rootdir):
            m = re.match('queue-(.*)', dir)
            if m:
                queue_name = m.group(1)
                queuedir = os.path.join(self.rootdir, dir)
                failures = [f for f in os.listdir(queuedir)
                                    if f.endswith(GAVE_UP_EXTENSION)]
                if len(failures):
                    print '* Gave up on %d documents in queue %r!' % (
                        len(failures), queue_name)
                    if (self.verbosity > 1):
                        for failure in failures:
                            print '  - %s' % failure

    def watch_pipeline_once(self):
        """return true if we're not done yet"""
        self.workers = Worker.get_all(self.rootdir)
        self.check_abandoned_jobs()
        changed, all_done = self.check_pipeline()
        if ((self.verbosity > 0) and
            (changed or all_done or self.verbosity > 1)):
            self.list_workers(clear_screen)
        else:
            self.check_memory()
        self.update_tputs()
        return not all_done

    def watch_pipeline(self):
        self.log('Monitoring SERIF pipeline.', 0)
        clear_screen = (self.verbosity>1)
        try:
            while self.watch_pipeline_once():
                if self.verbosity > 1:
                    time.sleep(3)
                else:
                    time.sleep(10)
        except KeyboardInterrupt:
            sys.stderr.write('Exiting because of keyboard interrupt...\n')
            sys.stderr.flush()
            self.workers = Worker.get_all(self.rootdir)
            num_alive = sum(w.is_alive() for w in self.workers)
            if num_alive:
                sys.stderr.write(KEYBOARD_INTERRUPT_MSG % dict(
                        num_alive=num_alive, prog=sys.argv[0]))
            return False
        except SerifStartupError as e:
            sys.stderr.write(str(e) + '\n')
            return False
        except Exception, e:
            sys.stderr.write('Exception while monitoring pipeline:\n')
            traceback.print_exc()
            sys.stderr.write('Continuing anyway (after 10 second delay)...\n')
            sys.stderr.flush()
            time.sleep(10)
        if self.verbosity >= 0:
            print
            self.list_workers(clear_screen)
            self.show_failures()

        return True

KEYBOARD_INTERRUPT_MSG = '''
%(num_alive)d processes are still running.

          To kill them, run: "%(prog)s kill"
  To resume monitoring, run: "%(prog)s run"
'''

class SerifStartupError(Exception):
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return self.value

######################################################################
# Helper Functions
######################################################################

def read_with_retry(filename, retries=3):
    """
    Read and return the contents of a given file.  If there is an
    error during reading, then try again the specified number of
    times.
    """
    for i in range(retries+1):
        if i<2: time.sleep(i*0.25)
        try:
            return open(filename, 'rb').read()
        except (IOError, OSError, ValueError), e:
            pass
            #print 'For %s got %s, retry' % (filename, e)
    return ''

_CLEAR_SCREEN = None
def clear_screen():
    global _CLEAR_SCREEN
    if _CLEAR_SCREEN is None:
        if sys.stdout.isatty():
            try:
                import curses
                curses.setupterm()
                _CLEAR_SCREEN = re.sub(r'\$<\d+>[/*]?', '',
                                      curses.tigetstr('clear') or '')
            except:
                _CLEAR_SCREEN = ''
        else:
            _CLEAR_SCREEN = ''
    sys.stdout.write(_CLEAR_SCREEN)

def start_daemon(cmd, pidfile, outfile=None):
    #print cmd
    try:
        pid = os.fork()
    except OSError, e:
        raise Exception, "%s [%d]" % (e.strerror, e.errno)

    if (pid == 0):
        os.setsid()
        process = subprocess.Popen(cmd, stdin=subprocess.PIPE,
                                   stdout=outfile,
                                   stderr=outfile,
                                   close_fds=True)
        # Record the PID (process id)
        pid = open(pidfile, 'wb')
        pid.write('%s\n' % process.pid)
        pid.close()
        os._exit(0)
    else:
        time.sleep(0.1)

def pid_exists(pid):
    """
    Check whether pid exists in the current process table.  The
    current implementation only supports unix-like operating systems
    (linux, os x).
    """
    if pid is None or pid < 0:
        return False
    try:
        os.kill(pid, 0)
    except OSError, e:
        return e.errno != errno.ESRCH
    return True

def get_process_info(pid):
    stats = collections.defaultdict(lambda: '?')
    if pid is not None:
        try:
            status = read_with_retry('/proc/%d/status' % pid)
            for m in re.finditer('(?m)^(\w+):[ \t]*(.*)[ \t]*$', status):
                stats[m.group(1)] = m.group(2)
        except IOError:
            pass
    return stats

def parse_memory(s):
    """
    Extract kb from a memory line, as expressed in /proc/pid/status.
    """
    return int(re.match('(\d+) kB', s).group(1))

def format_memory(s):
    if isinstance(s, int): s = '%d kB' % s
    m = re.match('(\d+) kB', s)
    if m:
        kb = int(m.group(1))
        if kb > 1024:
            return '%d MB' % (kb/1024.0)
    return s

def format_time(msec):
    if msec is None:
        return 'loading'
    elif msec < 1000:
        return '%d msec' % msec
    elif msec < 60*1000:
        return '%.1f sec' % (msec/1000.0)
    elif msec < 60*60*1000:
        return '%.1f min' % (msec/1000.0/60.0)
    else:
        return '%.1f hrs' % (msec/1000.0/60.0/60.0)


######################################################################
# Command-Line Interface
######################################################################

USAGE='''
%prog [options] command

Commands:
  * run........ Start all SERIF worker processes (if they are not
                already running), and then continue periodically
                monitoring the processes, restarting them if they die.
                Use "-v" for a continuous display of running processes.

  * start...... Start all SERIF worker processes.  If they have
                already been started, then do nothing.

  * stop....... Send an "exit" signal to all SERIF worker processes,
                and wait for them to exit.

  * kill....... Immediately kill all SERIF worker processes.

  * show....... Display the current status of SERIF workers.  Does
                not start or stop any workers.
'''

class CommandLineInterface(object):
    def __init__(self, pipeline):
        self.pipeline = pipeline
        self.parfile = None
        self.root = None
        self.build_parser()
        self.parse_args()

    def build_parser(self):
        self.parser = optparse.OptionParser(usage=USAGE)
        self.parser.add_option("-v", action='count', dest='verbose', default=0,
                               help="Generate more verbose output")
        self.parser.add_option("-q", action='count', dest='quiet', default=0,
                               help="Generate less verbose output")
        self.parser.add_option("--no-cleanup", action='store_false', dest='cleanup',
                               default=True, help="Do not remove worker "
                               "directories when stopping or killing workers")
        self.parser.add_option("--serif-bin", dest='serif_bin', metavar='PATH',
                               help="Path to Serif binary")
        if self.root is None:
            self.parser.add_option("--queue-dir", dest='root', metavar='DIR',
                                   help="Directory where queue is stored")
        if self.parfile is None:
            self.parser.add_option("--par", dest='par', metavar='FILE',
                                   help="Master parameter file")

    def parse_args(self):
        (self.options, args) = self.parser.parse_args()
        if len(args) == 0: args = ['run']
        if len(args) > 1: self.parser.error('Expected a single command')
        self.verbosity = (1 + self.options.verbose - self.options.quiet)
        self.cmd = args[0].lower()
        if self.options.serif_bin:
            Worker.set_serif_bin(self.options.serif_bin)
        if self.parfile is None:
            self.parfile = self.options.par
        if self.root is None:
            self.root = self.options.root

    def run(self):
        if self.parfile is None:
            self.parser.error("--par required")
        if self.root is None:
            self.parser.error("--queue-dir required")

        # Make these options:
        monitor = QueueMonitor(self.root, self.parfile,
                               self.verbosity, self.pipeline)

        # Perform the requested command.
        if self.cmd == 'run':
            monitor.watch_pipeline()
        elif self.cmd == 'show':
            monitor.check_abandoned_jobs()
            monitor.list_workers()
        elif self.cmd == 'start':
            monitor.check_abandoned_jobs()
            monitor.check_pipeline()
            monitor.list_workers()
        elif self.cmd == 'kill':
            monitor.kill_all(self.options.cleanup)
            monitor.check_abandoned_jobs()
        elif self.cmd == 'stop':
            monitor.close_all(self.options.cleanup)
            monitor.check_abandoned_jobs()
        else:
            self.parser.error("Unexpected command %s" % self.cmd)

if __name__ == '__main__':
    print "Copyright 2015 Raytheon BBN Technologies Corp.  "
    print 'All Rights Reserved.'
    print
    cli = CommandLineInterface(DEFAULT_PIPELINE)
    cli.run()
