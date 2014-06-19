set terminal pngcairo transparent enhanced font "arial,10" fontscale 1.0 size 800,600; set zeroaxis;;
set output "chart.png"
set datafile separator "," 
set border 4095 front linetype -1 linewidth 1.000
set style line 100  linetype 5 linecolor rgb "#f0e442"  linewidth 0.500 pointtype 5 pointsize default pointinterval 0
set samples 30
set isosamples 30
unset surface
set title "set pm3d hidden3d <linetype>: pm3d's much faster hidden3d variant"
set pm3d implicit at s
set pm3d interpolate 1,1 flush begin noftriangles hidden3d 100 corners2color mean
set xlabel "kernel size"
set ylabel "pixel"
set zlabel "time"
splot "data_points.csv"