/*! \file FootTrajectoryGenerationStandard.h
  \brief This object generate all the values for the foot trajectories.
   @ingroup foottrajectorygeneration

   Copyright (c) 2007, 
   @author Francois Keith, Olivier Stasse,
   
   JRL-Japan, CNRS/AIST

   All rights reserved.
   
   Redistribution and use in source and binary forms, with or without modification, 
   are permitted provided that the following conditions are met:
   
   * Redistributions of source code must retain the above copyright notice, 
   this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above copyright notice, 
   this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
   * Neither the name of the CNRS/AIST nor the names of its contributors 
   may be used to endorse or promote products derived from this software without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS 
   OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY 
   AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER 
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, 
   OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
   OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
   STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
   IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef _FOOT_TRAJECTORY_GENERATION_STANDARD_H_
#define _FOOT_TRAJECTORY_GENERATION_STANDARD_H_

/* Walking pattern generation related inclusions */
#include <walkGenJrl/walkGenJrl_API.h>
#include <walkGenJrl/FootTrajectoryGeneration/FootTrajectoryGenerationAbstract.h>
#include <walkGenJrl/Mathematics/PolynomeFoot.h>

namespace PatternGeneratorJRL
{

  /** @ingroup foottrajectorygeneration
      This class generates a trajectory for the swinging foot during single support phase.
      It uses a classical approach relying in polynome of 3rd orders for the position in the 
      orthogonal plan as well as the direction.For the height modification a 4th order polynome
      is used. Finally a landing and take off phase using an angular value (\f$\omega\f$).
  */
  class WALK_GEN_JRL_EXPORT FootTrajectoryGenerationStandard : public FootTrajectoryGenerationAbstract
  {
  public:

    /*!\name  Constants related to the direction for the generation of the polynomes. 
      @{ */
    
    /*! \brief along the frontal direction */
    const static unsigned int X_AXIS =0;
    /*! \brief along the left of the robot */
    const static unsigned int Y_AXIS = 1;
    /*! \brief along the vertical axis of the robot. */
    const static unsigned int Z_AXIS = 2;
    /*! \brief Along the direction of the robot*/
    const static unsigned int THETA_AXIS = 3;
    /*! \brief Angle used by the swinging foot for taking off. */
    const static unsigned int OMEGA_AXIS = 4;
    /*! \brief Angle used by the swinging foot for landing */
    const static unsigned int OMEGA2_AXIS = 5;

    /* @} */

    /*! Constructor: In order to compute some appropriate strategies,
      this class needs to extract specific details from the humanoid model. */
    FootTrajectoryGenerationStandard(SimplePluginManager *lSPM,dynamicsJRLJapan::HumanoidSpecificities *aHS);

    /*! Default destructor. */
    virtual ~FootTrajectoryGenerationStandard();

    /*! This method computes the position of the swinging foot during single support phase,
      and maintian a constant position for the support foot.
      It uses polynomial of 3rd order for the X-axis, Y-axis, 
      orientation in the X-Z axis, and orientation in the X-Y axis,
      and finally it uses a 4th order polynome for the Z-axis.
      
      @param SupportFootAbsolutePositions: Queue of absolute position for the support foot.
      This method will set the foot position at index CurrentAbsoluteIndex of the queue.
      This position is supposed to be constant.
      @param NoneSupportFootAbsolutePositions: Queue of absolute position for the swinging
      foot. This method will set the foot position at index NoneSupportFootAbsolutePositions
      of the queue. 
      @param CurrentAbsoluteIndex: Index in the queues of the foot position to be set.
      @param IndexInitial: Index in the queues which correspond to the starting point
      of the current single support phase.
      @param ModulatedSingleSupportTime: Amount of time where the foot is flat.
      @param StepType: Type of steps (for book-keeping).
    */
   virtual void UpdateFootPosition(deque<FootAbsolutePosition> &SupportFootAbsolutePositions,
				   deque<FootAbsolutePosition> &NoneSupportFootAbsolutePositions,
				   int CurrentAbsoluteIndex,  
				   int IndexInitial, 
				   double ModulatedSingleSupportTime,
				   int StepType,int LeftOrRight);
   
   /*! Initialize internal data structures.
     In this specific case, it is in charge of creating the polynomial structures.
    */
   virtual void InitializeInternalDataStructures();
   
   /*! Free internal data structures.
     In this specific case, it is in charge of freeing the polynomial data structures.
    */
   virtual void FreeInternalDataStructures();
   
   /*! This method specifies the parameters for each of the polynome used by this
     object. In this case, as it is used for the 3rd order polynome. The polynome to
     which those parameters are set is specified with PolynomeIndex. 
     It assumes an initial position and an initial speed set to zero. 
     @param[in] AxisReference: Set to which axis the parameters will be applied. 
     @param[in] TimeInterval: Set the time base of the polynome.
     @param[in] Position: Set the final position of the polynome at TimeInterval.
   */
   int SetParameters(int AxisReference,
		     double TimeInterval,
		     double Position);

   /*! This method specifies the parameters for each of the polynome used by this
     object. In this case, as it is used for the 3rd order polynome. The polynome to
     which those parameters are set is specified with PolynomeIndex. 
     @param[in] AxisReference: Set to which axis the parameters will be applied. 
     @param[in] TimeInterval: Set the time base of the polynome.
     @param[in] FinalPosition: Set the final position of the polynome at TimeInterval.
     @param[in] InitPosition: Initial position when computing the polynome at t=0.0.
     @param[in] InitSpeed: Initial speed when computing the polynome at t=0.0.
   */
   int SetParametersWithInitPosInitSpeed(int AxisReference,
					 double TimeInterval,
					 double FinalPosition,
					 double InitPosition,
					 double InitSpeed);

   /*! Fill an absolute foot position structure for a given time. */
   double ComputeAll(FootAbsolutePosition & aFootAbsolutePosition,
		     double Time);

   /*! Compute the value for a given polynome. */
   double Compute(unsigned int PolynomeIndex, double Time);

   /*! Compute the absolute foot position from the queue of relative positions. 
     There is not direct dependency with time.
    */
   void ComputingAbsFootPosFromQueueOfRelPos(deque<RelativeFootPosition> &RelativeFootPositions,
					     deque<FootAbsolutePosition> &AbsoluteFootPositions);

   /*! Methods to compute a set of positions for the feet according to the discrete time given in parameters
     and the phase of walking. 
     @{
   */
   
   /*! @} */
   

  protected:
   
   /*! \brief Polynomes for X and Y axis positions*/
   Polynome3 *m_PolynomeX,*m_PolynomeY;
   
   /*! \brief Polynome for X-Y orientation */
   Polynome3 *m_PolynomeTheta;

   /*! \brief Polynome for Y-Z orientation */
   Polynome3 *m_PolynomeOmega, *m_PolynomeOmega2;

   /*! \brief Polynome for Z axis position. */
   Polynome4 *m_PolynomeZ;
   

   /*! \brief Foot dimension. */
   double m_FootB, m_FootH, m_FootF;

   /*! \brief Position of the ankle in the left foot. */
   MAL_S3_VECTOR(m_AnklePositionLeft,double);

   /*! \brief Position of the ankle in the right foot. */
   MAL_S3_VECTOR(m_AnklePositionRight,double);

   

  };
  
  
};
#endif /* _FOOT_TRAJECTORY_GENERATION_ABSTRACT_H_ */

