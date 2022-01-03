#include "ECE_UAV.h"
#include <cmath>
#include<iostream>
#include<random>
#include<chrono>
#include<mutex>
#define M_PI 3.14


extern ECE_UAV uavs[15];
void threadFunction(ECE_UAV* pUAV)
{
	//each thread runs this function, to update postion, velocity and acceleration information about the UAV
	double* posVec;
	double posVec2[3] = {};
	double posVec3[3] = {};

	//start after initial 5s
	std::this_thread::sleep_for(std::chrono::seconds(5));
	posVec = pUAV->getPosition();

	//get unit vector in direction of vector towards centre of sphere
	posVec2[0] = 0.0 - posVec[0];
	posVec2[1] = 0.0 - posVec[1];
	posVec2[2] = 150.0 - posVec[2];
	double norm = sqrt(posVec2[0] * posVec2[0] + posVec2[1] * posVec2[1] + posVec2[2] * posVec2[2]);
	posVec3[0] = posVec2[0] / (norm);																//unit vectors
	posVec3[1] = posVec2[1]/ (norm);
	posVec3[2] = posVec2[2]/ (norm);

	//Force vectors
	double forceV[3] = {};
	forceV[0] = 20.0 * posVec3[0];
	forceV[1] = 20.0 * posVec3[1];
	forceV[2] = 20.0 * posVec3[2];

	//move towards centre of sphere..part 1
	do
	{
		//set acceleration
		pUAV->m_Acceleration[0] = forceV[0];
		pUAV->m_Acceleration[1] =  forceV[1];
		pUAV->m_Acceleration[2] =  forceV[2];	

		double vNew[3] = {};
		vNew[0] = pUAV->m_Velocity[0] + pUAV->m_Acceleration[0] * 0.010;
		vNew[1]= pUAV->m_Velocity[1] + pUAV->m_Acceleration[1] * 0.010;
		vNew[2]= pUAV->m_Velocity[2] + pUAV->m_Acceleration[2] * 0.010;
		
		double xNew[3] = {};
		xNew[0] = pUAV->m_Position[0] + pUAV->m_Velocity[0] + 0.5 * pUAV->m_Acceleration[0] * 0.000100;
		xNew[1] = pUAV->m_Position[1] + pUAV->m_Velocity[1] + 0.5 * pUAV->m_Acceleration[1] * 0.000100;
		xNew[2] = pUAV->m_Position[2] + pUAV->m_Velocity[2] + 0.5 * pUAV->m_Acceleration[2] * 0.000100;
				
		pUAV->position(xNew);
		double* TestPos = pUAV->getPosition();
	
		//if within radius of sphere, move to next function for surface motion
		double distDiff = sqrt(xNew[0] * xNew[0] + xNew[1] * xNew[1] + (xNew[2] - 150) * (xNew[2] - 150));
	
		if (distDiff- 33 <= 0) 
		{
			pUAV->m_bStop = true;
		}

		pUAV->m_Velocity[0] = vNew[0];
		pUAV->m_Velocity[1] = vNew[1];
		pUAV->m_Velocity[2] = vNew[2];

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	} while (!pUAV->m_bStop);

	//reached surface
	pUAV->m_bStop = false;
	
	//go to a random point along surface
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::mt19937 generator(seed);
	std::uniform_real_distribution<double> uniform01(0.0, 1.0);

	double theta = 2 * M_PI * uniform01(generator);
	double phi = acos(1 - 2 * uniform01(generator));
	double x = sin(phi) * cos(theta) * 33.0f;
	double y = sin(phi) * sin(theta) * 33.0f;
	double z = cos(phi) * 33.0f + 150.0;
	bool breakFor = false;

	//Keep track of time to exit after 60s
	std::chrono::steady_clock::time_point beginNow = std::chrono::steady_clock::now();

	double xRandInitial[3] = { x,y,z };
	for (int i = 0; i < 30; i++)
	{
		do
		{
			std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
			//check when 60 seconds have elapsed, break when 60 seconds up
			if (std::chrono::duration_cast<std::chrono::seconds>(end - beginNow).count() > 60)
			{
				breakFor = true;
				break;
			}
			//vector from uavpos to centre (0,0,150)-V1
			double* posV1 = pUAV->getPosition();			//current position of uav
			double normV1 = sqrt(((0 - posV1[0]) * (0 - posV1[0])) + ((0 - posV1[1]) * (0 - posV1[1])) + ((150 - posV1[2]) * (150 - posV1[2])));
			double unitV1[3] = { (posV1[0] - 0) / normV1, (posV1[1] - 0) / normV1, (posV1[2] - 150) / normV1 };			//unit vector1

			//vector from centre to random point
			double normV2 = sqrt(((0 - xRandInitial[0]) * (0 - xRandInitial[0])) + ((0 - xRandInitial[1]) * (0 - xRandInitial[1])) + ((150 - xRandInitial[2]) * (150 - xRandInitial[2])));
			double unitV2[3] = { (xRandInitial[0] - 0) / normV2, (xRandInitial[1] - 0) / normV2, (xRandInitial[2] - 150) / normV2 };			//unit vector 2

			//take cross product to get unit vector from origin outwards:
			//reference: https://www.cuemath.com/geometry/cross-product/

			double unitV3[3] = {};
			unitV3[0] = unitV1[1] * unitV2[2] - unitV1[2] * unitV2[1];
			unitV3[1] = unitV1[2] * unitV2[0] - unitV1[0] * unitV2[2];
			unitV3[2] = unitV1[0] * unitV2[1] - unitV1[1] * unitV2[0];

			//take cross product with uav position vector from origin to get tangent vector
			double unitV4_[3] = {};
			unitV4_[0] = (unitV3[1] * unitV1[2] - unitV3[2] * unitV1[1])*0.15;
			unitV4_[1] = (unitV3[2] * unitV1[0] - unitV3[0] * unitV1[2])*0.15;
			unitV4_[2] = (unitV3[0] * unitV1[1] - unitV3[1] * unitV1[0])*0.15;

			//normalize to get direction

			double normV4 = sqrt(unitV4_[0] * unitV4_[0] + unitV4_[1] * unitV4_[1] + unitV4_[2] * unitV4_[2]);
			//FInal direction vector of motion
			double unitV4[3] = { unitV4_[0] / normV4,unitV4_[1] / normV4, unitV4_[2] / normV4 };

			//move to new position--set velocities
			double new_Pos[3] = {};
			double Velocity = 5;
			double new_Vel[3] = { Velocity * unitV4[0],Velocity * unitV4[1],Velocity * unitV4[2] };


			//update velocities
			pUAV->m_Velocity[0] = new_Vel[0];
			pUAV->m_Velocity[1] = new_Vel[1];
			pUAV->m_Velocity[2] = new_Vel[2];
						
			//move...
			double xNew[3] = {};
			xNew[0] = pUAV->m_Position[0] + pUAV->m_Velocity[0] * 0.1;// +0.5 * pUAV->m_Acceleration[0] * 0.01;
			xNew[1] = pUAV->m_Position[1] + pUAV->m_Velocity[1] * 0.1;// +0.5 * pUAV->m_Acceleration[1] * 0.01;
			xNew[2] = pUAV->m_Position[2] + pUAV->m_Velocity[2] * 0.1;// +0.5 * pUAV->m_Acceleration[2] * 0.01;
	
			//update position
			pUAV->position(xNew);
	
			//calculate for Collision
			double *otherUAVpos;
			double* myPos;
			double dx, dy, dz, interDistance;
			for (int ii = 0; ii < 15; ii++)
			{
				
				if (&uavs[ii] != pUAV)
				{
					otherUAVpos= uavs[ii].getPosition();
					myPos= pUAV->getPosition();
					dx = otherUAVpos[0] - myPos[0];
					dy= otherUAVpos[1] - myPos[1];
					dz = otherUAVpos[2] - myPos[2];

					//Check if both UAVS close enough
					interDistance = sqrt(dx * dx + dy * dy + dz * dz);
					
					if (interDistance < 3)
					{
						//swap velocities
						double temp[3] = {};
						temp[0] = uavs[ii].m_Velocity[0];
						temp[1] = uavs[ii].m_Velocity[1];
						temp[2] = uavs[ii].m_Velocity[2];

						uavs[ii].m_Velocity[0] = pUAV->m_Velocity[0];
						uavs[ii].m_Velocity[1] = pUAV->m_Velocity[1];
						uavs[ii].m_Velocity[2] = pUAV->m_Velocity[2];

						pUAV->m_Velocity[0] = temp[0];
						pUAV->m_Velocity[1] = temp[1];
						pUAV->m_Velocity[2] = temp[2];
					}
				}
			}
			//If the UAV reaches its virtual point, generate a new virtual point
			double D_lim = sqrt(((xNew[0] - xRandInitial[0]) * (xNew[0] - xRandInitial[0])) + ((xNew[1] - xRandInitial[1]) * (xNew[1] - xRandInitial[1])) + ((xNew[2] - xRandInitial[2]) * (xNew[2] - xRandInitial[2])));
			if (D_lim < 10)
			{			
				pUAV->m_bStop = true;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(30));
		} while (!pUAV->m_bStop);

		if (breakFor)
		{
			break;
		}

		//update virtual points
		pUAV->m_bStop = false;
		std::uniform_real_distribution<double> uniform02(0.0, 1.0);
		theta = 2 * M_PI * uniform02(generator);
		phi = acos(1 - 2 * uniform02(generator));
		x = sin(phi) * cos(theta) * 33.0f;
		y = sin(phi) * sin(theta) * 33.0f;
		z = cos(phi) * 33.0f + 150.0;
		xRandInitial[0] = x;
		xRandInitial[1] = y;
		xRandInitial[2] = z;
	}
}

void ECE_UAV::start()
{
	//starts the threads
	m_KinematicsThread = std::thread(threadFunction, this);
}

void ECE_UAV::stop()
{
	m_bStop = true;
	if (m_KinematicsThread.joinable())
	{
		m_KinematicsThread.join();
	}
}

double* ECE_UAV::getPosition()
{
	return m_Position ;
}

