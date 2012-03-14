/*
 * DrudePusher.h
 *
 *  Created on: 10 Mar 2012
 *      Author: stefans
 */

#ifndef DRUDEPUSHER_H_
#define DRUDEPUSHER_H_

#include "../pushers/pusher.h"
#include "../Configuration.h"
#include "../Node.h"
#include "../Particle.h"
#include "BigParticle.h"
#include <vector>
#include <boost/foreach.hpp>
#include <cmath>
#include <iostream>

namespace treecode{
namespace pusher{

template<class Vec, class Mat>
class DrudePusher : public Pusher<Vec, Mat>{
public:
	DrudePusher(const Configuration<Vec>& config, BoundaryConditions<Vec,Mat>& bounds, DrudeMAC<Vec,Mat>& mac):
		config_(config), bounds_(bounds), mac_(mac){}

	std::pair<double, double> push_particles(std::vector<Particle<Vec,Mat>*> parts, Tree<Vec,Mat>& tree,
			BoundaryConditions<Vec,Mat>& bc,
			potentials::Precision prec, const AcceptanceCriterion<Vec, Mat>& mac){
		typedef Particle<Vec,Mat> part_t;

		tree.rebuild();

		std::pair<double, double> energies(0,0);
		BOOST_FOREACH(part_t* p, parts){
			p->updateVelocity(Vec(1,0));
			std::pair<double, double> p_energy = push_particle_recursive(p, tree, bc, prec, mac, config_.getTimestep());
			energies.first += p_energy.first;
			energies.second += p_energy.second;
		}
		return energies;
	}

private:
	std::pair<double, double> push_particle_recursive(Particle<Vec,Mat>* p, Tree<Vec,Mat>& tree,
			BoundaryConditions<Vec,Mat>& bc,
				potentials::Precision prec, const AcceptanceCriterion<Vec, Mat>& mac, double dt){
		typedef std::vector<Node<Vec,Mat>* > interaction_list;

		mac_.setTimestep(dt);

//		tree.rebuild();

		double ke = 0;
		interaction_list ilist;
		//Start from the particle's grandparent.
		tree.getInteractionList(*p, ilist, mac);

		if(ilist.size() > 0){
			double distance_to_travel = p->getVelocity().norm() * config_.getTimestep();
			for(typename interaction_list::iterator it = ilist.begin(); it < ilist.end(); it++){
				Node<Vec,Mat>* n = *it;
				BigParticle<Vec,Mat>* interacting_particle = dynamic_cast<BigParticle<Vec,Mat>*>(n->getParticles().front());
				if(!DrudeMAC<Vec,Mat>::willIntersect(*p, *interacting_particle, bounds_))
					continue;

				double distance_to_edge = DrudeMAC<Vec,Mat>::distanceToIntersection(*p, *interacting_particle, bounds_);
				if(distance_to_edge > distance_to_travel || distance_to_edge < 0)
					continue;
				//Move to edge of particle
				p->updatePosition(p->getVelocity() * dt * (distance_to_edge/distance_to_travel));
				bounds_.particleMoved(p);
				if(p->getPosition()[0] < -50 || p->getPosition()[1] < -50)
					std::cout << p->getPosition() << std::endl;

				//Reflect the velocity vector
				Vec disp_vec = p->getPosition() - interacting_particle->getPosition();
				disp_vec /= disp_vec.norm();
				Mat reflection_matrix = Mat::Identity() - 2.0 * disp_vec * disp_vec.transpose();
				p->setVelocity(reflection_matrix * p->getVelocity());
				//Now move forward again
				push_particle_recursive(p, tree, bc, prec, mac, dt * (1 - distance_to_edge / distance_to_travel));
			}
		}else{
			p->updatePosition(p->getVelocity() * dt);
		}
		ke += 0.5 * p->getMass() * p->getVelocity().squaredNorm();
		bounds_.particleMoved(p);

		return std::pair<double, double>(ke, 0);
	}


protected:
	const Configuration<Vec>& config_;
	BoundaryConditions<Vec,Mat>& bounds_;
	DrudeMAC<Vec,Mat>& mac_;
};

}
}

#endif /* DRUDEPUSHER_H_ */