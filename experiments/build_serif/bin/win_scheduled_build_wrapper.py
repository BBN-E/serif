###
# SERIF Windows Scheduled Build Wraper
#
# Runs the scheduled build and emails the results.
###

import email_wrapper
import runpy

def run_scheduled_build():
    """
    """

    # Execute the build which is just flat code, no method definitions
    build_globals = runpy.run_module("win_scheduled_build")

    # Retrieve the build logs
    log_output = ""
    for log_global in ('log_filename', 'err_filename', 'build_filename'):
        log_output += open(build_globals[log_global], 'rb').read()
    return log_output

# Run the above wrapper method and email the results
email_wrapper.run_and_send(
    "serif-regression@bbn.com",
    "serif-regression@bbn.com",
    "Windows Scheduled Build",
    run_scheduled_build,
)
