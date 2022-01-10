"""
Draw ascii-art histogram.
"""

from collections import defaultdict

def make_bargraph(values, min_value=None, max_value=None, bars=65):
    if hasattr(values, 'items'): values = values.items()

    # Automatic minimum & maximum values.
    if min_value is None:
        if isinstance(values[0], tuple):
            min_value = min(val[0] for val in values)
        else:
            min_value = min(values)
    if max_value is None:
        if isinstance(values[0], tuple):
            max_value = max(val[0] for val in values)
        else:
            max_value = max(values)
            
    bars = [0]*width
    dv = max_value - min_value
    if max_value == min_value:
        dv = max(1, max_value)

    for value in values:
        if isinstance(value, tuple):
            value, weight = value
        else:
            weight = 1.0
        value = max(min(value, max_value), min_value)
        bar = (value-x0)*dx
        bars[int(bar)] += weight

    return bars

def avg(values):
    return sum(values)/float(len(values))

def draw_histogram(values, min_value=None, max_value=None, width=65,
                   height=8, normalize=False, title=None,
                   combine=sum, max_bar=None):
    """
    @param values: A list of values, or a list of (value,weight) tuples.
    @param width, height: The size of the region used to draw bars;
        the actual returned string will be slightly larger to
        accomadate the axes.
    """
    if not values: return ''

    if hasattr(values, 'items'): values = values.items()
    if min_value is None:
        if isinstance(values[0], tuple):
            min_value = min(val[0] for val in values)
        else:
            min_value = min(values)
    if max_value is None:
        if isinstance(values[0], tuple):
            max_value = max(val[0] for val in values)
        else:
            max_value = max(values)
    dv = max_value - min_value
    if max_value == min_value:
        dv = max(1, max_value)
    ## avoid zero divisions, pretend zero-based graph if all values are the same
    dx = float(width-1) / dv
    x0 = min_value
    bars = [[] for i in range(width)]
    for value in values:
        if isinstance(value, tuple):
            value, weight = value
        else:
            weight = 1.0
        value = max(min(value, max_value), min_value)
        bar = (value-x0)*dx
        bars[int(bar)].append(weight)
    # Combine the bars.
    bars = [combine(bar) if bar else 0 for bar in bars]
    # Find the tallest bar, so we can scale to it.
    if max_bar is None: max_bar = max(bars)
    if max_bar == 0: return ''
    # Normalize, if requested.
    if normalize:
        bars = [bar/float(max_bar) for bar in bars]
        max_bar = 1
    # Draw the graph.
    result = ''
            
    if max_bar > 100: leftaxis = '%6d|'
    elif max_bar > 10: leftaxis = '%6.1f|'
    elif max_bar > 1: leftaxis = '%6.2f|'
    elif max_bar > 0.1: leftaxis = '%6.3f|'
    else: leftaxis = '%6.4f|'
    for line in range(height-1, -1, -1):
        result += leftaxis % (max_bar*float(line)/(height-1))
        for bar in bars:
            if bar <= 0 and line == 0:
                result += '_'
            elif float(bar)/max_bar >= float(line)/(height-1):
                result += ':'
            elif float(bar)/max_bar >= float(line-0.5)/(height-1):
                result += '.'
            else:
                result += ' '
        result += '\n'

    # Add the title.  Try to splice it into the graph if we can.
    if title:
        lines = result.split('\n')
        s, e = 7+width/2-(len(title)+2)/2, 7+width/2+(len(title)+3)/2
        if lines[0][s:e].strip():
            lines.insert(0, '%s' % title.center(width+14).rstrip())
        else:
            lines[0] = lines[0][:s] + (' %s ' % title) + lines[0][e:]
        result = '\n'.join(lines)

    # Axis label:
    result += ' '
    for col in range(0, max(1, width-15), 12):
        if dx > 100:  tick = '%.3f' % (col/dx+x0)
        elif dx > 10: tick = '%.2f' % (col/dx+x0)
        elif dx > 1:  tick = '%.1f' % (col/dx+x0)
        else: tick = '%d' % (col/dx+x0)
        assert len(tick) < 12
        result += tick.center(12)
    if (width-col-12) >= 0:
        result += ' '*(width-col-12) + ('%.1f' % max_value).center(12).rstrip()

    return result


# [xx] I could use pyterm here to do color bars.
BARS = [':._', 'Xx_', ':._', '|._', '#._']

def draw_bargraph(*value_sets, **kwargs):
    # Read keyword args:
    width = kwargs.pop('width', 65)
    height = kwargs.pop('height', 8)
    normalize = kwargs.pop('normalize', False)
    value_sort_key = kwargs.pop('value_sort_key', None)
    title = kwargs.pop('title', None)
    skip_key = kwargs.pop('skip_key', False)
    label_width = kwargs.pop('label_width', False)
    if kwargs: raise TypeError('Unpexpected arg %s' % kwargs.popitem()[0])

    # Collect all of the possible values.
    values = set()
    for value_set in value_sets:
        if hasattr(value_set, 'items'): value_set = value_set.items()
        for value in value_set:
            if isinstance(value, tuple): v = value[0]
            else: v = value
            values.add(v)
                
    # Construct the value bars.
    value_bars = dict((v, [0]*len(value_sets)) for v in values)
    for value_set_num, value_set in enumerate(value_sets):
        if hasattr(value_set, 'items'): value_set = value_set.items()
        for value in value_set:
            if isinstance(value, tuple): v, n = value
            else: v, n = value, 1
            value_bars[v][value_set_num] += n

    # Sort the values.
    values = sorted(values, key=value_sort_key)
    
    # Determine how much space to leave between bar groups.
    num_bars = len(values)*len(value_sets)
    if label_width:
        spacer = label_width - len(value_sets) + 1
    else:
        spacer = max(0, (width-num_bars) / (len(values)+1))
        if len(value_sets) > 1 and spacer==0: spacer = 1
        label_width = spacer+len(value_sets)-1

    # Decide whether to use a key or not.
    use_key = label_width < 3 and len(values) < 5
    
    # Determine the height of the tallest bar.
    max_bar = float(max(max(b for b in bars) for bars in value_bars.values()))
    if max_bar == 0: return ''
    if normalize:
        value_bars = dict((v,[b/max_bar for b in bars])
                          for (v,bars) in value_bars)
        max_bar = 1.0

    # Add a title, if requested.
    result = ''
    if title:
        result += '       %s\n' % title.center(width).rstrip()

    # Determine how many values we can show in the perscribed width.
    num_values_per_subgraph = (width-spacer)/(len(value_sets)+spacer)
    for start_pos in range(0, len(values), num_values_per_subgraph):
        subgraph_values = values[start_pos:start_pos+num_values_per_subgraph]
        result += _draw_bargraph(subgraph_values, value_bars, max_bar,
                                 height, spacer, value_sets, use_key)

    if use_key and not skip_key:
        for i, label in enumerate(values):
            result += '  %2s: %s\n' % (i+1, label)
        
    return result

def _draw_bargraph(values, value_bars, max_bar, height,
                   spacer, value_sets, use_key):
    result = ''
    # Prepare the Y-axis label.
    if max_bar > 100: leftaxis = '%6d|'
    elif max_bar > 10: leftaxis = '%6.1f|'
    elif max_bar > 1: leftaxis = '%6.2f|'
    elif max_bar > 0.1: leftaxis = '%6.3f|'
    else: leftaxis = '%6.4f|'

    # Draw the graph.
    for line in range(height-1, -1, -1):
        result += leftaxis % (max_bar*float(line)/(height-1))
        for col, value in enumerate(values):
            if col == 0:
                result += ('_' if line==0 else ' ')*spacer
            for bar_num, bar in enumerate(value_bars[value]):
                BAR = BARS[bar_num%len(BARS)]
                if bar == 0 and line == 0:
                    result += BAR[2]
                elif float(bar)/max_bar >= float(line+0.5)/(height-1):
                    result += BAR[0]
                elif float(bar)/max_bar >= float(line)/(height-1):
                    result += BAR[1]
                else:
                    result += ' '
            result += ('_' if line==0 else ' ')*spacer
        result += '\n'
        
    # Value axis labels:
    labels = ['%s' % v for v in values]
    label_width = spacer+len(value_sets)-1
    if not spacer: label_width = 1
    use_key = (label_width < 3) and len(values) < 5
    if use_key:
        labels = [str(i+1) for i in range(len(labels))]
    while any(labels):
        result += ' '*(7+label_width/2)
        for label in labels:
            if len(label) < label_width:
                label = label.center(label_width)
            if spacer:
                result += label[:label_width].ljust(label_width+1)
            else:
                result += label[:label_width].ljust(1)
        result += '\n'
        labels = [label[label_width:] for label in labels]
        
    return result

def horiz_bargraph(values, width=75, height=8, title=None, indent=2,
                   label_width=None): # (label_width is ignored)
    if hasattr(values, 'items'): values = values.items()
    bars = defaultdict(float)
    for value in values:
        if isinstance(value, tuple):
            bars[value[:-1]] += value[-1]
        else:
            bars[(value,)] += 1

    max_bar = max(bars.values())
    
    # Make a template for the left axis
    max_value_width = max(len(_horiz_label(v)) for v in bars)+indent
    leftaxis = '%'+str(max_value_width)+'s |'

    # Determine how much space to leave for the bars.
    max_count_width = max(len(str(bar)) for bar in bars.values())
    bar_width = max(10, width-max_value_width-max_count_width-2)
    
    result = ''
    if title:
        result += '%s\n' % title.center(width).rstrip()

    scale = bar_width/max_bar
    result += _horiz_bargraph(bars, leftaxis, scale)
    if result.endswith((leftaxis%'') + '\n'):
        result = result[:-max_value_width-3]
    return result

def _horiz_label(value):
    return ':'.join('%s' % piece for piece in value)

def _horiz_bargraph(bars, leftaxis, scale, prefix=()):
    result = ''
    keys = sorted(set([bar[:len(prefix)+1] for bar in bars
                       if bar[:len(prefix)]==prefix]))
    
    for i, key in enumerate(keys):
        # This key has its own bar:
        if key in bars:
            bar = bars[key]
            result += leftaxis % _horiz_label(key)
            result += '='*(int(bar*scale) or (bar>0))
            result += ' %s' % bar
            result += '\n'
        # This key has subkeys:
        else:
            if i == 0 and prefix!=(): result += (leftaxis%'') + '\n'
            result += _horiz_bargraph(bars, leftaxis, scale, key)
            result += (leftaxis%'') + '\n'
                
    return result
    

    
if __name__ == '__main__':
    v = ([(i,i) for i in range(10)])
    print draw_histogram(v, width=10, height=10, min_value=0)

    print draw_bargraph('the the and a ama i oo ugly'.split(),
                        'the the and the a a'.split())
    import random
    print horiz_bargraph([random.randint(0,5) for i in range(1000)])
    print horiz_bargraph([(random.randint(0,5),random.randint(0,5), 1)
                           for i in range(1000)])
    
