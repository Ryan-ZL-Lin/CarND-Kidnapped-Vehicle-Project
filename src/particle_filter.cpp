/**
 * particle_filter.cpp
 *
 * Created on: Dec 12, 2016
 * Author: Tiffany Huang
 */

#include "particle_filter.h"

#include <math.h>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "helper_functions.h"

using namespace std;

using std::string;
using std::vector;
using std::normal_distribution;
using std::numeric_limits;

void ParticleFilter::init(double x, double y, double theta, double std[]) {
  /**
   * TODO: Set the number of particles. Initialize all particles to 
   *   first position (based on estimates of x, y, theta and their uncertainties
   *   from GPS) and all weights to 1. 
   * TODO: Add random Gaussian noise to each particle.
   * NOTE: Consult particle_filter.h for more information about this method 
   *   (and others in this file).
   */
  
  num_particles = 60;  // TODO: Set the number of particles
  std::default_random_engine gen;
  double std_x = std[0];
  double std_y = std[1];
  double std_theta = std[2];
  normal_distribution<double> dist_x(x, std_x);
  normal_distribution<double> dist_y(y, std_y);
  normal_distribution<double> dist_theta(theta, std_theta);

  for(int i = 0; i < num_particles; i++){

    double sample_x, sample_y, sample_theta;
    sample_x = dist_x(gen);
    sample_y = dist_y(gen);
    sample_theta = dist_theta(gen);

    Particle p;
    p.x = sample_x;
    p.y = sample_y;
    p.theta = sample_theta;
    p.id = i;
    p.weight = 1;

    particles.push_back(p);
  }

  this->is_initialized = true;
  return;

}

void ParticleFilter::prediction(double delta_t, double std_pos[], 
                                double velocity, double yaw_rate) {
  /**
   * TODO: Add measurements to each particle and add random Gaussian noise.
   * NOTE: When adding noise you may find std::normal_distribution 
   *   and std::default_random_engine useful.
   *  http://en.cppreference.com/w/cpp/numeric/random/normal_distribution
   *  http://www.cplusplus.com/reference/random/default_random_engine/
   */
  std::default_random_engine gen;
  double std_x = std_pos[0];
  double std_y = std_pos[1];
  double std_theta = std_pos[2];
  
  double x_predict, y_predict, theta_predict;

  for (unsigned int i = 0; i < particles.size() ; i++){
    if(fabs(yaw_rate) <= 0.00001){
      x_predict = particles[i].x + velocity * delta_t * cos(particles[i].theta);
      y_predict = particles[i].y + velocity * delta_t * sin(particles[i].theta);
      theta_predict = particles[i].theta;
    }
    else{
      x_predict = particles[i].x + (velocity / yaw_rate) * (sin(particles[i].theta + yaw_rate * delta_t) - sin(particles[i].theta));
      y_predict = particles[i].y + (velocity / yaw_rate) * (cos(particles[i].theta) - cos(particles[i].theta + yaw_rate * delta_t));
      theta_predict = particles[i].theta + yaw_rate * delta_t;
    }

    normal_distribution<double> dist_x(x_predict, std_x);
    normal_distribution<double> dist_y(y_predict, std_y);
    normal_distribution<double> dist_theta(theta_predict, std_theta);

    double sample_x, sample_y, sample_theta;
    sample_x = dist_x(gen);
    sample_y = dist_y(gen);
    sample_theta = dist_theta(gen);

    particles[i].x = sample_x;
    particles[i].y = sample_y;
    particles[i].theta = sample_theta;

  }
}

void ParticleFilter::dataAssociation(vector<LandmarkObs> predicted, 
                                     vector<LandmarkObs>& observations) {
  /**
   * TODO: Find the predicted measurement that is closest to each 
   *   observed measurement and assign the observed measurement to this 
   *   particular landmark.
   * NOTE: this method will NOT be called by the grading code. But you will 
   *   probably find it useful to implement this method and use it as a helper 
   *   during the updateWeights phase.
   */

  for (unsigned int i = 0; i < observations.size(); i++){
    double pre_distance = numeric_limits<double>::max();
    int final_landmark_id = -1;

    for (unsigned int j = 0; j < predicted.size(); j++){
      double distance = dist(observations[i].x, observations[i].y, predicted[j].x, predicted[j].y);

      if (distance < pre_distance){
        pre_distance = distance;
        final_landmark_id = predicted[j].id;
      }
    }
    observations[i].id = final_landmark_id;
  }
}

void ParticleFilter::updateWeights(double sensor_range, double std_landmark[], 
                                   const vector<LandmarkObs> &observations, 
                                   const Map &map_landmarks) {
  /**
   * TODO: Update the weights of each particle using a mult-variate Gaussian 
   *   distribution. You can read more about this distribution here: 
   *   https://en.wikipedia.org/wiki/Multivariate_normal_distribution
   * NOTE: The observations are given in the VEHICLE'S coordinate system. 
   *   Your particles are located according to the MAP'S coordinate system. 
   *   You will need to transform between the two systems. Keep in mind that
   *   this transformation requires both rotation AND translation (but no scaling).
   *   The following is a good resource for the theory:
   *   https://www.willamette.edu/~gorr/classes/GeneralGraphics/Transforms/transforms2d.htm
   *   and the following is a good resource for the actual equation to implement
   *   (look at equation 3.33) http://planning.cs.uiuc.edu/node99.html
   */
  for (unsigned int i = 0; i < particles.size(); i++){
    //filter map landmarks baed on the sensor range limitation
    vector<LandmarkObs> valid_Landmarks;
    for(unsigned int j = 0; j < map_landmarks.landmark_list.size(); j++){
      if(fabs(particles[i].x - map_landmarks.landmark_list[j].x_f) <= sensor_range && fabs(particles[i].y - map_landmarks.landmark_list[j].y_f)){
        valid_Landmarks.push_back(LandmarkObs{map_landmarks.landmark_list[j].id_i, map_landmarks.landmark_list[j].x_f, map_landmarks.landmark_list[j].y_f});
      }
    }
    
    vector<LandmarkObs> transformed_obs;
    //transform coordinate system
    for (unsigned int k = 0; k < observations.size(); k++){
      //Homogenous Transformation 
      double x_map = particles[i].x + (cos(particles[i].theta) * observations[k].x) - (sin(particles[i].theta) * observations[k].y);
      double y_map = particles[i].y + (sin(particles[i].theta) * observations[k].x) + (cos(particles[i].theta) * observations[k].y);
      transformed_obs.push_back(LandmarkObs{observations[k].id, x_map, y_map});
    }

    dataAssociation(valid_Landmarks, transformed_obs);

    //Calculate weight for each particle
    particles[i].weight = 1.0;

    for (unsigned int m = 0; m < transformed_obs.size(); m++){
      LandmarkObs mu;
      for(unsigned int n = 0; n < valid_Landmarks.size(); n++){
        if(transformed_obs[m].id == valid_Landmarks[n].id){
          mu = valid_Landmarks[n];
        }
      }
      double importance_weight = multi_prob(std_landmark[0], std_landmark[1], transformed_obs[m].x, transformed_obs[m].y, mu.x, mu.y);
      particles[i].weight *= importance_weight;
    }
  }
}

void ParticleFilter::resample() {
  /**
   * TODO: Resample particles with replacement with probability proportional 
   *   to their weight. 
   * NOTE: You may find std::discrete_distribution helpful here.
   *   http://en.cppreference.com/w/cpp/numeric/random/discrete_distribution
   */
  vector<Particle> new_particles;
  vector<double> new_weights;
  std::default_random_engine gen;

  for(int i = 0; i < num_particles; i++){
    new_weights.push_back(particles[i].weight);
  }
  discrete_distribution<> dist(new_weights.begin(), new_weights.end());

  for(int i = 0; i < num_particles; i++){
    new_particles.push_back(particles[dist(gen)]);
  }

  particles = new_particles;

}

void ParticleFilter::SetAssociations(Particle& particle, 
                                     const vector<int>& associations, 
                                     const vector<double>& sense_x, 
                                     const vector<double>& sense_y) {
  // particle: the particle to which assign each listed association, 
  //   and association's (x,y) world coordinates mapping
  // associations: The landmark id that goes along with each listed association
  // sense_x: the associations x mapping already converted to world coordinates
  // sense_y: the associations y mapping already converted to world coordinates
  particle.associations= associations;
  particle.sense_x = sense_x;
  particle.sense_y = sense_y;
}

string ParticleFilter::getAssociations(Particle best) {
  vector<int> v = best.associations;
  std::stringstream ss;
  copy(v.begin(), v.end(), std::ostream_iterator<int>(ss, " "));
  string s = ss.str();
  s = s.substr(0, s.length()-1);  // get rid of the trailing space
  return s;
}

string ParticleFilter::getSenseCoord(Particle best, string coord) {
  vector<double> v;

  if (coord == "X") {
    v = best.sense_x;
  } else {
    v = best.sense_y;
  }

  std::stringstream ss;
  copy(v.begin(), v.end(), std::ostream_iterator<float>(ss, " "));
  string s = ss.str();
  s = s.substr(0, s.length()-1);  // get rid of the trailing space
  return s;
}