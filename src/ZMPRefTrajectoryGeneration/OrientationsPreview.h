/*
 * OrientationsPreview.h
 *
 *  Created on: Apr 26, 2010
 *      Author: andrei
 */

#ifndef ORIENTATIONSPREVIEW_H_
#define ORIENTATIONSPREVIEW_H_



/*! STL includes */
#include <deque>

/*! Framework includes */
#include <PreviewControl/SupportState.h>

/*! Framework includes */
#include <walkGenJrl/PGTypes.h>

namespace PatternGeneratorJRL
{
class OrientationsPreview {
public:
	OrientationsPreview(const double & SamplingPeriod,
			const unsigned int & SamplingsPreviewed,
			const double & SSPeriod);
	~OrientationsPreview();

	void previewOrientations(double &Time,
			std::deque<double> &PreviewedSupportAngles,
			double &AngVelTrunkConst, double &PreviewedTrunkAngle,
			COMState_t &TrunkState, SupportState * Support,
			std::deque<FootAbsolutePosition> &LeftFootAbsolutePositions,
			std::deque<FootAbsolutePosition> &RightFootAbsolutePositions);

	void verifyAccelerationOfHipJoint(const ReferenceAbsoluteVelocity_t &Ref, double &AngVelTrunkConst,
			const COMState_t &TrunkState, const SupportState * Support);
	//TODO 0: Unused variables in OrientationsPreview.h
	//	double* CurAngVelCoH, double* TrunkAngle, double* TimeLimit, double* Time,
	//	int* SupportPhase, int* SupportFoot, double* SupportAngle, double* RightFootAngle, double* LeftFootAngle,
	//	/*Outside*/double* T, double* MaxIntAngleTrunk, double* MaxExtAngleTrunk, double* SSDuration, double* N, double* MaxAngVelFoot,
	//	double* MaxIntAngleFeet);

private:
	/*! Angular limitations of the hip joints*/
	double m_lLimitLeftHipYaw, m_uLimitLeftHipYaw, m_lLimitRightHipYaw, m_uLimitRightHipYaw;

	/*! Maximal acceleration of a hip joint*/
	double m_uaLimitHipYaw;

	/*! Upper crossing angle limit between the feet*/
	double m_uLimitFeet;

	/*! Maximal velocity of a foot*/
	double m_uvLimitFoot;

	/*! Single-support duration*/
	double m_SSPeriod;

	/*! Number of sampling in a preview window*/
	double m_N;

	/*! Time between two samplings*/
	double m_T;




	unsigned int m_FullDebug;


};
};
#endif /* ORIENTATIONSPREVIEW_H_ */
