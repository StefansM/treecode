/*
 * PeriodicBoundary.h
 *
 *  Created on: 20 Jan 2012
 *      Author: stefans
 */

#ifndef CULLINGBOUNDARY_H_
#define CULLINGBOUNDARY_H_

#include "BoundaryConditions.h"
#include "../Configuration.h"
#include <vector>

namespace treecode {

template <class Vec, class Mat, class RNG>
class CullingBoundary : public PeriodicBoundary<Vec,Mat> {
public:
	/**
	 * @class CullingBoundary
	 * @brief Boundaries that reset velocity of particles.
	 * Whenever a particle moves out of the system, it is translated back into the system,
	 * and the velocity vector of the particle is reset.
	 */

	/**
	 * @brief Create a new set of culling boundary conditions.
	 *
	 * To describe which boundaries to reset, pass an std::vector<bool>
	 * with the same number of entries as dimensions the system is
	 * operating in.
	 *
	 * For example: if min_reset = {false, false, true} and
	 * max_reset = {true, false, false}, the velocity vector
	 * will be reset when going going past the minimum value in the
	 * z axis and past the maximum value on the x axis.
	 *
	 * @param conf		Configuration.
	 * @param origin	Origin of system.
	 * @param length	Length of each side of the system.
	 * @param vel_dist	Velocity distribution
	 * @param min_reset	Describes which boundaries to reset at.
	 * @param max_reset Describes which boundaries to reset at.
	 */
	CullingBoundary(const Configuration<Vec>& conf, const Vec origin, double length,
			const distribution::VectorDistribution<RNG,Vec>& vel_dist,
			bool* min_reset, bool* max_reset,
			RNG& rng):
		PeriodicBoundary<Vec,Mat>(conf, origin, length), 	//Parent class
		vel_dist_(vel_dist),								//Velocity distribution
		min_reset_(min_reset), max_reset_(max_reset),		//Min/max reset
		rng_(rng), right_edge_(Vec::Zero()), left_edge_(Vec::Zero())
		{}

	virtual ~CullingBoundary(){}

	/**
	 * @brief Shift particle at edges of box.
	 * When a particle is moved, this method should be called. It translates
	 * the particle to the opposite edge of the simulation region when it
	 * moves out.
	 *
	 * @param p	Particle to move.
	 */
	virtual void particleMoved(treecode::Particle<Vec,Mat>* p){
		typedef PeriodicBoundary<Vec,Mat> parent;

		Vec translation_vector = Vec::Zero();

		for (int i = 0; i < p->getPosition().rows(); i++) {
			double component = p->getPosition()[i];
			if(component < parent::origin_[i]){
				left_edge_[i] ++;
				translation_vector[i] = parent::length_;
				//Reset velocity if told to
				if(min_reset_[i])
					p->setVelocity(vel_dist_.getVector(rng_));
			}
			else if(component > parent::origin_[i] + parent::length_){
				right_edge_[i]++;
				translation_vector[i] = -parent::length_;
				//Reset velocity if told to
				if(max_reset_[i])
					p->setVelocity(vel_dist_.getVector(rng_));
			}
		}
		p->updatePosition(translation_vector);
	}

	void timestepOver(){
		for(int i = 0; i < left_edge_.rows(); i++){
			std::cerr << left_edge_[i] << "\t" << right_edge_[i] << "\t";
		}
		std::cerr << std::endl;

		left_edge_ = Vec::Zero();
		right_edge_ = Vec::Zero();
	}

protected:
	const distribution::VectorDistribution<RNG,Vec>& vel_dist_;
	bool *min_reset_, *max_reset_;
	RNG& rng_;
	Vec right_edge_, left_edge_;
};

} /* namespace treecode */
#endif /* CULLINGBOUNDARY_H_ */