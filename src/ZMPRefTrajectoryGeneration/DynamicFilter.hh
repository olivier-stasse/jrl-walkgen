#ifndef DYNAMICFILTER_HH
#define DYNAMICFILTER_HH

// metapod includes
#include <metapod/models/hrp2_14/hrp2_14.hh>
#include <MotionGeneration/ComAndFootRealizationByGeometry.hh>
#include <metapod/algos/jac_point_chain.hh>

#ifndef METAPOD_TYPEDEF
#define METAPOD_TYPEDEF
  typedef double LocalFloatType;
  typedef metapod::Spatial::ForceTpl<LocalFloatType> Force_HRP2_14;
  typedef metapod::hrp2_14<LocalFloatType> Robot_Model;
  typedef metapod::Nodes< Robot_Model, Robot_Model::BODY >::type RootNode;
  typedef metapod::Nodes< Robot_Model, Robot_Model::l_ankle >::type LankleNode;
  typedef metapod::Nodes< Robot_Model, Robot_Model::r_ankle >::type RankleNode;

  typedef metapod::jac_point_chain < Robot_Model,
    Robot_Model::l_ankle, Robot_Model::BODY,0,true,false> Jacobian_LF;
  typedef metapod::jac_point_chain < Robot_Model,
    Robot_Model::r_ankle, Robot_Model::BODY,0,true,false> Jacobian_RF;

#endif

namespace PatternGeneratorJRL
{

  class DynamicFilter
  {
  public: // Public methods

    // to use the vector of eigen used by metapod
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    /// \brief
    DynamicFilter(SimplePluginManager *SPM,
                  CjrlHumanoidDynamicRobot *aHS
                  );
    ~DynamicFilter();
    /// \brief
    int filter(
        COMState & lastCtrlCoMState,
        FootAbsolutePosition & lastCtrlLeftFoot,
        FootAbsolutePosition & lastCtrlRightFoot,
        deque<COMState> & inputCOMTraj_deq_,
        deque<ZMPPosition> inputZMPTraj_deq_,
        deque<FootAbsolutePosition> & inputLeftFootTraj_deq_,
        deque<FootAbsolutePosition> & inputRightFootTraj_deq_,
        deque<COMState> & outputDeltaCOMTraj_deq_
        );

    void init(
        double currentTime,
        double controlPeriod,
        double interpolationPeriod,
        double PG_T,
        double previewWindowSize,
        double CoMHeight,
        FootAbsolutePosition supportFoot,
        COMState inputCoM
        );

    /// \brief atomic function
    void InverseKinematics(
        const COMState & inputCoMState,
        const FootAbsolutePosition & inputLeftFoot,
        const FootAbsolutePosition & inputRightFoot,
        MAL_VECTOR_TYPE(double) & configuration,
        MAL_VECTOR_TYPE(double) & velocity,
        MAL_VECTOR_TYPE(double) & acceleration,
        double samplingPeriod,
        int stage,
        int iteration);

    /// \brief atomic function allow to compute
    void ComputeZMPMB(
        double samplingPeriod,
        const COMState & inputCoMState,
        const FootAbsolutePosition & inputLeftFoot,
        const FootAbsolutePosition & inputRightFoot,
        vector<double> & ZMPMB,
        int stage,
        int iteration);

    void stage0INstage1();

    /// \brief Preview control on the ZMPMBs computed
    int OptimalControl(
        deque<ZMPPosition> & inputdeltaZMP_deq,
        deque<COMState> & outputDeltaCOMTraj_deq_);

  private: // Private methods

    /// \brief calculate, from the CoM computed by the preview control,
    ///    the corresponding articular position, velocity and acceleration
    void InverseKinematics(
        const COMState & lastCtrlCoMState,
        const FootAbsolutePosition & lastCtrlLeftFoot,
        const FootAbsolutePosition & lastCtrlRightFoot,
        const deque<COMState> & inputCOMTraj_deq_,
        const deque<FootAbsolutePosition> & inputLeftFootTraj_deq_,
        const deque<FootAbsolutePosition> & inputRightFootTraj_deq_);

    /// \brief Apply the RNEA on the robot model and over the whole trajectory
    /// given by the function "filter"
    void InverseDynamics(deque<ZMPPosition> inputZMPTraj_deq);

    /// \brief Compute the ZMPMB according to a configuration
    void ComputeZMPMB(
        MAL_VECTOR_TYPE(double) & configuration,
        MAL_VECTOR_TYPE(double) & velocity,
        MAL_VECTOR_TYPE(double) & acceleration,
        vector<double> & ZMPMB);

    void computeWaist(const FootAbsolutePosition & inputLeftFoot);

    // -------------------------------------------------------------------

    /// \brief Debug function
    void printAlongTime(deque<COMState> & inputCOMTraj_deq_,
                    deque<ZMPPosition> inputZMPTraj_deq_,
                    deque<FootAbsolutePosition> & inputLeftFootTraj_deq_,
                    deque<FootAbsolutePosition> & inputRightFootTraj_deq_,
                    deque<COMState> & outputDeltaCOMTraj_deq_
                    );
    /// \brief Debug function
    void printBuffers(deque<COMState> & inputCOMTraj_deq_,
                    deque<ZMPPosition> inputZMPTraj_deq_,
                    deque<FootAbsolutePosition> & inputLeftFootTraj_deq_,
                    deque<FootAbsolutePosition> & inputRightFootTraj_deq_,
                    deque<COMState> & outputDeltaCOMTraj_deq_
                    );
    /// \brief Debug function
    double filterprecision(double adb);


  public: // The accessors

    /// \brief setter :
    inline void setCurrentTime(double time)
    {currentTime_ = time ;}

    inline void setControlPeriod(double controlPeriod)
    {controlPeriod_ = controlPeriod ;}

    inline void setInterpolationPeriod(double interpolationPeriod)
    {interpolationPeriod_ = interpolationPeriod ; return ;}

    inline void setPGPeriod(double PG_T)
    {PG_T_ = PG_T ;}

    inline void setPreviewWindowSize_(double previewWindowSize)
    { previewWindowSize_ = previewWindowSize; }

    /// \brief getter :
    inline ComAndFootRealizationByGeometry * getComAndFootRealization()
    { return comAndFootRealization_;};

    inline double getCurrentTime()
    {return currentTime_ ;}

    inline double getControlPeriod()
    {return controlPeriod_ ;}

    inline double getInterpolationPeriod()
    {return interpolationPeriod_ ;}

    inline double getPreviewWindowSize_()
    {return previewWindowSize_ ;}

    inline void getPCerror_(double & errx, double & erry)
    { errx = sxzmp_ ; erry = syzmp_ ; }

  private: // Private members

    /// \brief Time variables
    /// -----------------------------------
      /// \brief Current time of the PG.
      double currentTime_ ;

      /// \brief control period of the PG host
      double controlPeriod_;

      /// \brief Interpolation Period for the dynamic filter
      double interpolationPeriod_ ;

      /// \brief Sampling period of the PG host
      double PG_T_;

      /// \brief size of the previw window in second
      double previewWindowSize_ ;

      //------------------------------------------------------
      /// \brief Contain the number of control points
      unsigned int NCtrl_;

      /// \brief Contain the number of interpolation points
      /// inside the Sampling period of the PG host
      unsigned int NbI_ ;

      /// \brief Nb. samplings inside preview window
      unsigned int PG_N_ ;

    /// \brief Inverse Kinematics variables
    /// -----------------------------------
      /// \brief Store a reference to the object to solve posture resolution.
      ComAndFootRealizationByGeometry * comAndFootRealization_;

      /// \brief Buffers for the Inverse Kinematics
      std::vector <MAL_VECTOR_TYPE(double)> configurationTraj_ ;
      std::vector <MAL_VECTOR_TYPE(double)> velocityTraj_ ;
      std::vector <MAL_VECTOR_TYPE(double)> accelerationTraj_ ;
      MAL_VECTOR_TYPE(double) previousConfiguration_ ;
      MAL_VECTOR_TYPE(double) previousVelocity_ ;
      MAL_VECTOR_TYPE(double) previousAcceleration_ ;
      MAL_VECTOR_TYPE(double) aCoMState_;
      MAL_VECTOR_TYPE(double) aCoMSpeed_;
      MAL_VECTOR_TYPE(double) aCoMAcc_;
      MAL_VECTOR_TYPE(double) aLeftFootPosition_;
      MAL_VECTOR_TYPE(double) aRightFootPosition_;

      /// \brief used to compute the ZMPMB from only
      /// com and feet position from outside of the class
      MAL_VECTOR_TYPE(double) ZMPMBConfiguration_ ;
      MAL_VECTOR_TYPE(double) ZMPMBVelocity_ ;
      MAL_VECTOR_TYPE(double) ZMPMBAcceleration_ ;

      /// \brief data of the previous iteration
      bool PreviousSupportFoot_ ; // 1 = left ; 0 = right ;
      Robot_Model::confVector prev_q_, prev_dq_, prev_ddq_;
      double CWx, CWy;

    /// \brief Inverse Dynamics variables
    /// ---------------------------------
      /// \brief Initialize the robot with the autogenerated files
      /// by MetapodFromUrdf
      Robot_Model robot_,robot_2;

      /// \brief Initialize the robot leg jacobians with the
      /// autogenerated files by MetapodFromUrdf
      Jacobian_LF::Jacobian jacobian_rf_ ;
      Jacobian_RF::Jacobian jacobian_lf_ ;

      /// \brief force acting on the CoM of the robot expressed
      /// in the Euclidean Frame
      Force_HRP2_14 m_force ;

      /// \brief Set of configuration vectors (q, dq, ddq, torques)
      Robot_Model::confVector q_, dq_, ddq_;

      /// \brief Used to eliminate the initiale difference between
      /// the zmp and the zmpmb
      bool Once_ ;
      double DInitX_, DInitY_ ;

      /// \brief Buffers the ZMP Multibody computed
      /// from the inverse Dynamics, and the difference between
      /// this zmp and the reference one.
      deque< vector<double> > ZMPMB_vec_ ;
      std::deque<ZMPPosition> deltaZMP_deq_ ;

    /// \brief Optimal Control variables
    /// --------------------------------
      /// \brief Pointer to the Preview Control object.
      PreviewControl *PC_;
      double sxzmp_ , syzmp_ ;
      double deltaZMPx_, deltaZMPy_ ;
      double CoMHeight_ ;

      /// \brief State of the Preview control.
      MAL_MATRIX(deltax_,double);
      MAL_MATRIX(deltay_,double);
  };

}

#endif // DYNAMICFILTER_HH
