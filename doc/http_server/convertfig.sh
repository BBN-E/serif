#!/bin/env sh

convert server_design_figure_1.pdf -trim -resize 600x2000 \
    server_thread_class_diagram.png 
convert server_design_figure_2.pdf -trim -resize 600x2000 \
    server_control_flow_example.png 
