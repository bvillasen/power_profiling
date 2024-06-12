#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <fstream> 
#include <chrono>
#include <vector>
#include <unistd.h>

#include "rocm_smi/rocm_smi.h"

std::string getCurrentTimestamp()
{
    // auto currentTime = std::chrono::system_clock::now();
    // auto transformed = currentTime.time_since_epoch().count() / 1000000;
    // auto millis = transformed % (int64_t)1e3;

    auto currentTime = std::chrono::high_resolution_clock::now();
    auto timestamp = std::chrono::time_point_cast<std::chrono::microseconds>(currentTime);
    int microseconds = timestamp.time_since_epoch().count() % 1000000;
        
    char buffer[80];
    std::time_t tt;
    tt = std::chrono::system_clock::to_time_t ( currentTime );
    auto timeinfo = localtime (&tt);
    strftime (buffer,80,"%F %H:%M:%S",timeinfo);
    sprintf(buffer, "%s.%06d",buffer,microseconds);

    return std::string(buffer);
}


void get_cray_power(int dev_indx, int &power, int64_t &time_stamp ) {

    std::string cmd = "cat /sys/cray/pm_counters/accel";
    cmd.append( std::to_string(dev_indx) ) ;
    cmd.append( "_power" );

    std::string result;
    char buffer[128];
    FILE* pipe = popen(cmd.c_str(), "r");
    if (pipe) {
        while (!feof(pipe)) {
            if (fgets(buffer, 128, pipe) != nullptr) {
                result += buffer;
            }
        }
        pclose(pipe);
    }
    // std::cout << "Cray counter: " << result;

    std::stringstream ss(result);
    std::string token, s_power, s_timestamp;
    char delimiter = ' ';
    int indx = 0;
    while (std::getline(ss, token, delimiter)) {
        if ( indx == 0 ) s_power = token;
        if ( indx == 2 ) s_timestamp = token;
        indx += 1; 
    }
    power = stoi(s_power);
    time_stamp = stoll(s_timestamp);
    // std::cout << "power: " << power << " timestamp: " << time_stamp << std::endl;
}

int main(int argc, char* argv[]) {
  
  std::string output_file_name = argv[1];
  double time = std::stod(argv[2]);
  double sampling_freq = std::stod(argv[3]); //Hz
  // if ( argc >= 4 ) std::vector devices<int> = parse_devices_argument( argv[3] );
  std::vector<int> devices = { 0 };
  // std::vector<int> devices = { 0, 1, 2, 3 };
  
  int n_gpus = devices.size();
  int n_samples = static_cast<int>( time * sampling_freq );
  uint64_t sampling_period = static_cast<uint64_t>( 1.0/sampling_freq * 1e6 ); //microsecs
  
  std::cout << "Power Measure." << std::endl; 
  std::cout << "Output file: " << output_file_name << std::endl;
  std::cout << "N samples: " << n_samples << std::endl; 
  std::cout << "Sampling time: " << time <<  " seconds." << std::endl; 
  std::cout << "Sampling period: " << sampling_period/1000 <<  " milliseconds." << std::endl; 

  std::ofstream out_file(output_file_name);
  if (!out_file.is_open()) {
    std::cerr << "Error opening the output file: " << output_file_name << std::endl;
    return 1;
  }
  
  rsmi_status_t status, status_p, status_ps;
  uint32_t n_devices;
  status = rsmi_init(0);
  status = rsmi_num_monitor_devices(&n_devices);

  std::vector<std::string> times(n_samples);
  std::vector<std::vector<int>> rsmi_power_measurements(n_samples, std::vector<int>(n_gpus, 0));
  std::vector<std::vector<int>> cray_power_measurements(n_samples, std::vector<int>(n_gpus, 0));

  uint64_t power, socket_power;
  RSMI_POWER_TYPE power_type;
  int i_power_type;

  for (int dev_indx=0; dev_indx < n_gpus; dev_indx++) {
    std::cout << "dev_indx: " << dev_indx << std::endl;
    status_p = rsmi_dev_power_get( 2*dev_indx, &power, &power_type);
    if ( status_p == RSMI_STATUS_SUCCESS ) std::cout << "rocm-smi: power_get success" << std::endl;
    else if ( status_p == RSMI_STATUS_NOT_SUPPORTED ) std::cout << "rocm-smi: power_get not supported" << std::endl;
    else if ( status_p == RSMI_STATUS_INVALID_ARGS ) std::cout << "rocm-smi: power_get invalid arguments" << std::endl;

    status_ps = rsmi_dev_current_socket_power_get( 2*dev_indx, &socket_power );
    if ( status_ps == RSMI_STATUS_SUCCESS ) std::cout << "rocm-smi: current_socket_power_get success" << std::endl;
    else if ( status_ps == RSMI_STATUS_NOT_SUPPORTED ) std::cout << "rocm-smi: current_socket_power_get not supported" << std::endl;
    else if ( status_ps == RSMI_STATUS_INVALID_ARGS ) std::cout << "rocm-smi: current_socket_power_get invalid arguments" << std::endl;

    status = rsmi_dev_power_get( 2*dev_indx, &power, &power_type);
    if (power_type == RSMI_AVERAGE_POWER) i_power_type = 0;
    else if ( power_type == RSMI_CURRENT_POWER ) i_power_type = 1;
    else i_power_type = -1;
  }

  std::cout << "Starting power profiling at " << getCurrentTimestamp() << std::endl;
  
  int cray_power;
  int64_t cray_timestamp;
  int dev_id;
  for ( int sample_indx=0; sample_indx < n_samples; sample_indx++ ){
    times[sample_indx] = getCurrentTimestamp();
    for (int dev_indx=0; dev_indx < n_gpus; dev_indx++) {
      dev_id = devices[dev_indx];
      status_p = rsmi_dev_power_ave_get (2*dev_id, 0, &power);
      rsmi_power_measurements[sample_indx][dev_indx] = static_cast<int>(power/1e6);
      
      get_cray_power( dev_id, cray_power, cray_timestamp );
      cray_power_measurements[sample_indx][dev_indx] = cray_power;
    }

    // std::cout << "\nTime: " << times[iter] << " Power [W]: ";
    // for (int dev_indx=0; dev_indx<n_gpus; dev_indx++ ) std::cout << " " << rsmi_power_measurements[iter][dev_indx];
    // std::cout << std::endl;

    usleep( sampling_period );
  }

  std::cout << "Finished power profiling at " << getCurrentTimestamp() << std::endl;
  
  std::cout << "Writing measurements to output file." << std::endl;
  for ( int sample_indx=0; sample_indx < n_samples; sample_indx++ ){

    out_file << times[sample_indx] << " ";
    for ( int dev_indx=0; dev_indx<n_gpus; dev_indx++ ){
      out_file << rsmi_power_measurements[sample_indx][dev_indx] << " ";
      out_file << cray_power_measurements[sample_indx][dev_indx] << " ";
    }
    out_file << std::endl;

  }


  status = rsmi_shut_down();
  return 0;
}