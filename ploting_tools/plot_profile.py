import sys, os
import numpy as np
import matplotlib.pyplot as plt


def load_power_profile_file( file_name ):
  print( f'Loading file: {file_name}')
  file = open( file_name, 'r' )
  text = file.readlines()
  n_lines = len(text)
  dates, times, time_sec, rsmi_power, cray_power = [], [], [], [], []
  for line in text:
    date, time, r_power, c_power, nl = line.split(' ')
    dates.append(date)
    times.append(time)
    h, m, s = [ float(x)  for x in time.split(':') ]
    t_secs = 3600*h + 60*m + s
    time_sec.append(t_secs)
    rsmi_power.append(float(r_power))
    cray_power.append(float(c_power)) 
  return { 'date':dates, 'time':times, 'time_sec':np.array(time_sec), 
          'rsmi_power':np.array(rsmi_power), 'cray_power':np.array(cray_power) }

output_dir = '/mnt/c/Users/bvillase/work/projects/power_profiling/figures/'



n_devices = 2
sample_freqs = [10, 100, 1000]

data_all = []
for sample_freq in sample_freqs:
  file_name = f'../measurements/cholla_hydro/n{n_devices}/power_measurements_{sample_freq}Hz.txt'
  data_power = load_power_profile_file( file_name )
  figure_name = f'power_cholla_n{n_devices}.png'
  data_all.append( data_power )


nrows, ncols = 1, 3 
h_scale_factor = 0.7
figure_width  = 3 *ncols
figure_height = figure_width * h_scale_factor
font_size = 12
legend_size = 10
fig_text_size = 6
text_color = 'black'

n_figs = nrows * ncols
fig, ax_l = plt.subplots(nrows=nrows, ncols=ncols, figsize=(figure_width*ncols,figure_height*nrows))
plt.subplots_adjust( hspace = 0.15, wspace=0.15)


for i, sample_freq in enumerate( sample_freqs ):
  data_power = data_all[i]
  time = data_power['time_sec'] - data_power['time_sec'][0] 
  rsmi_power = data_power['rsmi_power']
  cray_power = data_power['cray_power']

  rsmi_mean = rsmi_power.mean()
  cray_mean = cray_power.mean()

  ax = ax_l[i]
  lw=.5
  ax.plot( time, cray_power, lw=lw, c='C0', label='cray counters'  )
  ax.plot( time, rsmi_power, lw=lw, c='C1', label='rocm-smi query' )

  ax.plot( [time[0], time[-1]], [cray_mean, cray_mean], ls='--', c='C0' )
  ax.plot( [time[0], time[-1]], [rsmi_mean, rsmi_mean], ls='--', c='C1'  )

  # idle = 95
  # ax.plot( [time[0], time[-1]], [idle, idle], ls='--', c='k' )

  # ax.text(1.0, 0.57, 'Mean: 140 W',
  #         verticalalignment='bottom', horizontalalignment='right',
  #         transform=ax.transAxes, color=text_color, fontsize=fig_text_size)

  # ax.text(0.97, 0.055, 'Idle: 95 W',
  #         verticalalignment='bottom', horizontalalignment='right',
  #         transform=ax.transAxes, color=text_color, fontsize=fig_text_size)

  ax.legend( frameon=False, fontsize=legend_size )
  ax.set_xlabel( 'Time [sec]', fontsize=font_size, color=text_color  )
  ax.set_ylabel( 'Power [W]', fontsize=font_size, color=text_color  )
  ax.set_title(f'Cholla - MI250X ({n_devices} GCD) - Sampling freq={sample_freq} Hz', fontsize=10)

  ax.set_ylim(0, 600)

figure_name = f'{output_dir}{figure_name}'
fig.savefig( figure_name, bbox_inches='tight', dpi=300, facecolor=fig.get_facecolor() )
print( f'Saved Figure: {figure_name}' )

