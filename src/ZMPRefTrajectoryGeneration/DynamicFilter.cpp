#include "DynamicFilter.hh"
#include <iomanip>
using namespace std;
using namespace PatternGeneratorJRL;
using namespace metapod;

DynamicFilter::DynamicFilter(
    SimplePluginManager *SPM,
    CjrlHumanoidDynamicRobot *aHS): stage0_(0) , stage1_(1)
{
  controlPeriod_ = 0.0 ;
  interpolationPeriod_ = 0.0 ;
  previewWindowSize_ = 0.0 ;
  PG_T_ = 0.0 ;
  NbI_ = 0.0 ;
  NCtrl_ = 0.0;
  PG_N_ = 0.0 ;

  cjrlHDR_ = aHS ;

  comAndFootRealization_ = new ComAndFootRealizationByGeometry((PatternGeneratorInterfacePrivate*) SPM );
  comAndFootRealization_->setHumanoidDynamicRobot(cjrlHDR_);
  comAndFootRealization_->SetHeightOfTheCoM(CoMHeight_);
  comAndFootRealization_->setSamplingPeriod(interpolationPeriod_);
  comAndFootRealization_->Initialization();

  PC_ = new PreviewControl(
        SPM,OptimalControllerSolver::MODE_WITH_INITIALPOS,false);
  CoMHeight_ = 0.0 ;

  deltaZMP_deq_.clear();
  ZMPMB_vec_.clear();

  MAL_VECTOR_RESIZE(aCoMState_,6);
  MAL_VECTOR_RESIZE(aCoMSpeed_,6);
  MAL_VECTOR_RESIZE(aCoMAcc_,6);
  MAL_VECTOR_RESIZE(aLeftFootPosition_,5);
  MAL_VECTOR_RESIZE(aRightFootPosition_,5);
  MAL_MATRIX_RESIZE(deltax_,3,1);
  MAL_MATRIX_RESIZE(deltay_,3,1);

  comAndFootRealization_->SetPreviousConfigurationStage0(
        cjrlHDR_->currentConfiguration());
  comAndFootRealization_->SetPreviousVelocityStage0(
        cjrlHDR_->currentVelocity());


  sxzmp_.clear();
  syzmp_.clear();
  deltaZMPx_.clear();
  deltaZMPy_.clear();

  upperPartIndex.clear();

  walkingHeuristic_ = false ;
}

DynamicFilter::~DynamicFilter()
{
  if (PC_!=0){
      delete PC_;
      PC_ = 0 ;
    }
  if (comAndFootRealization_!=0){
      delete comAndFootRealization_;
      comAndFootRealization_ = 0 ;
    }
}

void DynamicFilter::setRobotUpperPart(const MAL_VECTOR_TYPE(double) & configuration,
                                      const MAL_VECTOR_TYPE(double) & velocity,
                                      const MAL_VECTOR_TYPE(double) & acceleration)
{
  for ( unsigned int i = 0 ; i < upperPartIndex.size() ; ++i )
    {
      upperPartConfiguration_(upperPartIndex[i])  = configuration(upperPartIndex[i]);
      upperPartVelocity_(upperPartIndex[i])       = velocity(upperPartIndex[i]);
      upperPartAcceleration_(upperPartIndex[i])   = acceleration(upperPartIndex[i]);
    }
  return ;
}

/// \brief Initialise all objects, to be called just after the constructor
void DynamicFilter::init(
    double controlPeriod,
    double interpolationPeriod,
    double PG_T,
    unsigned int PG_N,
    double previewWindowSize,
    COMState inputCoMState)
{
  controlPeriod_ = controlPeriod ;
  interpolationPeriod_ = interpolationPeriod ;
  PG_T_ = PG_T ;
  previewWindowSize_ = previewWindowSize ;

  NbI_ = (double)PG_T_/interpolationPeriod_;

  NCtrl_ = (int)round(PG_T_/controlPeriod_) ;

  PG_N_ = PG_N ;


  CoMHeight_ = inputCoMState.z[0] ;
  PC_->SetPreviewControlTime (previewWindowSize_);
  PC_->SetSamplingPeriod (controlPeriod_);
  PC_->SetHeightOfCoM(CoMHeight_);
  PC_->ComputeOptimalWeights(OptimalControllerSolver::MODE_WITH_INITIALPOS);

  deltaZMP_deq_.resize( (int)round((PG_N_-1)*NCtrl_));
  ZMPMB_vec_.resize( (int)round(PG_N_*NbI_), vector<double>(2));
  zmpmb_i_.resize( PG_N_*NCtrl_, vector<double>(2));

  MAL_VECTOR_RESIZE(aCoMState_,6);
  MAL_VECTOR_RESIZE(aCoMSpeed_,6);
  MAL_VECTOR_RESIZE(aCoMAcc_,6);
  MAL_VECTOR_RESIZE(aLeftFootPosition_,5);
  MAL_VECTOR_RESIZE(aRightFootPosition_,5);
  MAL_MATRIX_RESIZE(deltax_,3,1);
  MAL_MATRIX_RESIZE(deltay_,3,1);

  MAL_VECTOR_FILL(aCoMState_,0.0);
  MAL_VECTOR_FILL(aCoMSpeed_,0.0);
  MAL_VECTOR_FILL(aCoMAcc_,0.0);
  MAL_VECTOR_FILL(aLeftFootPosition_,0.0);
  MAL_VECTOR_FILL(aRightFootPosition_,0.0);
  MAL_MATRIX_FILL(deltax_,0.0);
  MAL_MATRIX_FILL(deltay_,0.0);

  upperPartConfiguration_ = cjrlHDR_->currentConfiguration() ;
  previousUpperPartConfiguration_ = cjrlHDR_->currentConfiguration() ;
  upperPartVelocity_ = cjrlHDR_->currentVelocity() ;
  previousUpperPartVelocity_ = cjrlHDR_->currentVelocity() ;
  upperPartAcceleration_ = cjrlHDR_->currentAcceleration() ;

  ZMPMBConfiguration_ = cjrlHDR_->currentConfiguration() ;
  ZMPMBVelocity_      = cjrlHDR_->currentVelocity() ;
  ZMPMBAcceleration_  = cjrlHDR_->currentAcceleration() ;
  previousZMPMBConfiguration_ = cjrlHDR_->currentConfiguration() ;
  previousZMPMBVelocity_      = cjrlHDR_->currentVelocity() ;

  comAndFootRealization_->SetHeightOfTheCoM(CoMHeight_);
  comAndFootRealization_->setSamplingPeriod(interpolationPeriod_);
  comAndFootRealization_->Initialization();
  comAndFootRealization_->SetPreviousConfigurationStage0(ZMPMBConfiguration_);
  comAndFootRealization_->SetPreviousVelocityStage0(ZMPMBVelocity_);

  comAndFootRealization_->SetPreviousConfigurationStage1(ZMPMBConfiguration_);
  comAndFootRealization_->SetPreviousVelocityStage1(ZMPMBVelocity_);

  comAndFootRealization_->SetPreviousConfigurationStage2(ZMPMBConfiguration_);
  comAndFootRealization_->SetPreviousVelocityStage2(ZMPMBVelocity_);

  sxzmp_.resize(NCtrl_,0.0);
  syzmp_.resize(NCtrl_,0.0);
  deltaZMPx_.resize(NCtrl_,0.0);
  deltaZMPy_.resize(NCtrl_,0.0);

  upperPartIndex.resize(2+2+7+7);
  for (unsigned int i = 0 ; i < upperPartIndex.size() ; ++i )
    {
      upperPartIndex[i]=i+18;
    }
  return ;
}

int DynamicFilter::OffLinefilter(
    const deque<COMState> &inputCOMTraj_deq_,
    const deque<ZMPPosition> &inputZMPTraj_deq_,
    const deque<FootAbsolutePosition> &inputLeftFootTraj_deq_,
    const deque<FootAbsolutePosition> &inputRightFootTraj_deq_,
    const vector< MAL_VECTOR_TYPE(double) > & UpperPart_q,
    const vector< MAL_VECTOR_TYPE(double) > & UpperPart_dq,
    const vector< MAL_VECTOR_TYPE(double) > & UpperPart_ddq,
    deque<COMState> & outputDeltaCOMTraj_deq)
{
  unsigned int N = inputCOMTraj_deq_.size() ;
  ZMPMB_vec_.resize(N) ;
  deltaZMP_deq_.resize(N);

  setRobotUpperPart(UpperPart_q[0],UpperPart_dq[0],UpperPart_ddq[0]);

  for(unsigned int i = 0 ; i < N ; ++i )
    {
      ComputeZMPMB(interpolationPeriod_,inputCOMTraj_deq_[i],inputLeftFootTraj_deq_[i],
                   inputRightFootTraj_deq_[i], ZMPMB_vec_[i] , 1 , i);
    }
  for (unsigned int i = 0 ; i < N ; ++i)
    {
      deltaZMP_deq_[i].px = inputZMPTraj_deq_[i].px - ZMPMB_vec_[i][0] ;
      deltaZMP_deq_[i].py = inputZMPTraj_deq_[i].py - ZMPMB_vec_[i][1] ;
    }
  OptimalControl(deltaZMP_deq_,outputDeltaCOMTraj_deq) ;

  return 0;
}

int DynamicFilter::OnLinefilter(
    const deque<COMState> & inputCOMTraj_deq_,
    const deque<ZMPPosition> inputZMPTraj_deq_,
    const deque<FootAbsolutePosition> & inputLeftFootTraj_deq_,
    const deque<FootAbsolutePosition> & inputRightFootTraj_deq_,
    deque<COMState> & outputDeltaCOMTraj_deq_)
{
  int currentIteration = 20 ;
  unsigned int N = (int)round(PG_N_ * NbI_) ;
  ZMPMB_vec_.resize( N , vector<double>(2,0.0));
  for(unsigned int i = 0 ; i < N ; ++i)
  {
      ComputeZMPMB(interpolationPeriod_,
                   inputCOMTraj_deq_[i],
                   inputLeftFootTraj_deq_[i],
                   inputRightFootTraj_deq_[i],
                   ZMPMB_vec_[i],
                   stage1_,
                   currentIteration);
      if(i == (NbI_-1) || (i==0 && NbI_<1) )
        {
          previousZMPMBConfiguration_ = ZMPMBConfiguration_;
          previousZMPMBVelocity_      = ZMPMBVelocity_     ;
        }
  }

  int inc = (int)round(interpolationPeriod_/controlPeriod_) ;
  unsigned int N1 = N*inc+1 ;
  zmpmb_i_.resize( N1 , vector<double>(2) ) ;
  for(unsigned int i = 0 ; i < N-1 ; ++i)
    {
      zmpmb_i_[i*inc] = ZMPMB_vec_[i] ;
    }
  zmpmb_i_.back() = ZMPMB_vec_.back() ;
  for(unsigned int i = 0 ; i < N-1 ; ++i)
  {
    double xA = zmpmb_i_[i*inc][0] ;
    double yA = zmpmb_i_[i*inc][1] ;
    double tA = i*inc ;
    double xB = zmpmb_i_[(i+1)*inc][0] ;
    double yB = zmpmb_i_[(i+1)*inc][1] ;
    double tB = (i+1)*inc ;
    for(int j = 1 ; j < inc ; ++j)
      {
        double t = tA+j ;
        zmpmb_i_[(i*inc)+j][0] = xA + (t-tA)*(xB-xA)/(tB-tA) ;
        zmpmb_i_[(i*inc)+j][1] = yA + (t-tA)*(yB-yA)/(tB-tA) ;
      }
  }

  comAndFootRealization_->SetPreviousConfigurationStage1(previousZMPMBConfiguration_);
  comAndFootRealization_->SetPreviousVelocityStage1(previousZMPMBVelocity_);

  deltaZMP_deq_.resize(N1);
  for (unsigned int i = 0 ; i < N1 ; ++i)
    {
      deltaZMP_deq_[i].px = inputZMPTraj_deq_[i].px - zmpmb_i_[i][0] ;
      deltaZMP_deq_[i].py = inputZMPTraj_deq_[i].py - zmpmb_i_[i][1] ;
    }
  OptimalControl(deltaZMP_deq_,outputDeltaCOMTraj_deq_) ;

  return 0 ;
}

// #############################
int DynamicFilter::zmpmb(
          MAL_VECTOR_TYPE(double)& configuration,
          MAL_VECTOR_TYPE(double)& velocity,
          MAL_VECTOR_TYPE(double)& acceleration,
          vector<double> & zmpmb)
{
  InverseDynamics(configuration,velocity,acceleration);
  ExtractZMP(zmpmb);
  return 0 ;
}

//##################################
void DynamicFilter::InverseKinematics(
    const COMState & inputCoMState,
    const FootAbsolutePosition & inputLeftFoot,
    const FootAbsolutePosition & inputRightFoot,
    MAL_VECTOR_TYPE(double)& configuration,
    MAL_VECTOR_TYPE(double)& velocity,
    MAL_VECTOR_TYPE(double)& acceleration,
    double samplingPeriod,
    int stage,
    int iteration)
{

  // lower body !!!!! the angular quantities are set in degree !!!!!!
  aCoMState_(0) = inputCoMState.x[0];       aCoMSpeed_(0) = inputCoMState.x[1];
  aCoMState_(1) = inputCoMState.y[0];       aCoMSpeed_(1) = inputCoMState.y[1];
  aCoMState_(2) = inputCoMState.z[0];       aCoMSpeed_(2) = inputCoMState.z[1];
  aCoMState_(3) = inputCoMState.roll[0];    aCoMSpeed_(3) = inputCoMState.roll[1];
  aCoMState_(4) = inputCoMState.pitch[0];   aCoMSpeed_(4) = inputCoMState.pitch[1];
  aCoMState_(5) = inputCoMState.yaw[0];     aCoMSpeed_(5) = inputCoMState.yaw[1];

  aCoMAcc_(0) = inputCoMState.x[2];         aLeftFootPosition_(0) = inputLeftFoot.x;
  aCoMAcc_(1) = inputCoMState.y[2];         aLeftFootPosition_(1) = inputLeftFoot.y;
  aCoMAcc_(2) = inputCoMState.z[2];         aLeftFootPosition_(2) = inputLeftFoot.z;
  aCoMAcc_(3) = inputCoMState.roll[2];      aLeftFootPosition_(3) = inputLeftFoot.theta;
  aCoMAcc_(4) = inputCoMState.pitch[2];     aLeftFootPosition_(4) = inputLeftFoot.omega;
  aCoMAcc_(5) = inputCoMState.yaw[2];

  aRightFootPosition_(0) = inputRightFoot.x;
  aRightFootPosition_(1) = inputRightFoot.y;
  aRightFootPosition_(2) = inputRightFoot.z;
  aRightFootPosition_(3) = inputRightFoot.theta;
  aRightFootPosition_(4) = inputRightFoot.omega;

  comAndFootRealization_->setSamplingPeriod(samplingPeriod);
  comAndFootRealization_->ComputePostureForGivenCoMAndFeetPosture(
        aCoMState_, aCoMSpeed_, aCoMAcc_,
        aLeftFootPosition_, aRightFootPosition_,
        configuration, velocity, acceleration,
        iteration, stage);

  // upper body
  if (walkingHeuristic_)
    {
      upperPartConfiguration_         = configuration           ;
      upperPartVelocity_              = velocity                ;
      upperPartAcceleration_          = acceleration            ;
      previousUpperPartConfiguration_ = upperPartConfiguration_ ;
      previousUpperPartVelocity_      = upperPartVelocity_      ;

      MAL_VECTOR_FILL(upperPartVelocity_        , 0.0 ) ;
      MAL_VECTOR_FILL(upperPartAcceleration_    , 0.0 ) ;
      MAL_VECTOR_FILL(previousUpperPartVelocity_, 0.0 ) ;

    }

  for ( unsigned int i = 0 ; i < upperPartIndex.size() ; ++i )
    {
      configuration(upperPartIndex[i])= upperPartConfiguration_(upperPartIndex[i]);
      velocity(upperPartIndex[i]) = upperPartVelocity_(upperPartIndex[i]) ;
      acceleration(upperPartIndex[i]) = upperPartAcceleration_(upperPartIndex[i]) ;
    }

  return;
}

void DynamicFilter::InverseDynamics(
    MAL_VECTOR_TYPE(double)& configuration,
    MAL_VECTOR_TYPE(double)& velocity,
    MAL_VECTOR_TYPE(double)& acceleration
    )
{
  cjrlHDR_->currentConfiguration(configuration);
  cjrlHDR_->currentVelocity(velocity);
  cjrlHDR_->currentAcceleration(acceleration);
  cjrlHDR_->computeCenterOfMassDynamics();
  return ;
}

void DynamicFilter::ExtractZMP(vector<double> & zmpmb)
{
  MAL_S3_VECTOR_TYPE(double) zmpmb_HDR = cjrlHDR_->zeroMomentumPoint();
  zmpmb.resize(3);
  for (unsigned int i = 0 ; i < 3 ; ++i)
    zmpmb[i] = zmpmb_HDR(i) ;
  return ;
}

void DynamicFilter::stage0INstage1()
{
  comAndFootRealization_->SetPreviousConfigurationStage1(comAndFootRealization_->GetPreviousConfigurationStage0());
  comAndFootRealization_->SetPreviousVelocityStage1(comAndFootRealization_->GetPreviousVelocityStage0());
  return ;
}

void DynamicFilter::ComputeZMPMB(
    double samplingPeriod,
    const COMState & inputCoMState,
    const FootAbsolutePosition & inputLeftFoot,
    const FootAbsolutePosition & inputRightFoot,
    vector<double> & ZMPMB,
    unsigned int stage,
    unsigned int iteration)
{
  InverseKinematics( inputCoMState, inputLeftFoot, inputRightFoot,
                     ZMPMBConfiguration_, ZMPMBVelocity_, ZMPMBAcceleration_,
                     samplingPeriod, stage, iteration) ;

  InverseDynamics(ZMPMBConfiguration_, ZMPMBVelocity_, ZMPMBAcceleration_);

  ExtractZMP(ZMPMB);

  return ;
}

int DynamicFilter::OptimalControl(
    deque<ZMPPosition> & inputdeltaZMP_deq,
    deque<COMState> & outputDeltaCOMTraj_deq_)
{
  assert(PC_->IsCoherent());

  outputDeltaCOMTraj_deq_.resize(NCtrl_);
  double sxzmp     = sxzmp_.back() ;
  double syzmp     = syzmp_.back() ;
  double deltaZMPx = 0.0 ;
  double deltaZMPy = 0.0 ;
  // calcul of the preview control along the "deltaZMP_deq_"
  for (unsigned i = 0 ; i < NCtrl_ ; i++ )
  {
    PC_->OneIterationOfPreview(deltax_,deltay_,
                               sxzmp,syzmp,
                               inputdeltaZMP_deq,i,
                               deltaZMPx, deltaZMPy, false);

    sxzmp_[i] = sxzmp ;
    syzmp_[i] = syzmp ;
    deltaZMPx_[i] = deltaZMPx ;
    deltaZMPy_[i] = deltaZMPy ;

    for(int j=0;j<3;j++)
    {
      outputDeltaCOMTraj_deq_[i].x[j] = deltax_(j,0);
      outputDeltaCOMTraj_deq_[i].y[j] = deltay_(j,0);
    }
  }
  // test to verify if the Kajita PC diverged
  for (unsigned int i = 0 ; i < NCtrl_ ; i++)
    {
      for(int j=0;j<3;j++)
        {
          if ( outputDeltaCOMTraj_deq_[i].x[j] == outputDeltaCOMTraj_deq_[i].x[j] ||
               outputDeltaCOMTraj_deq_[i].y[j] == outputDeltaCOMTraj_deq_[i].y[j] )
            {}
          else{
              cout << "kajita2003 preview control diverged\n" ;
              return -1 ;
            }
        }
    }
  return 0 ;
}

// TODO finish the implementation of a better waist tracking
void DynamicFilter::computeWaist(const FootAbsolutePosition & inputLeftFoot)
{
//  Eigen::Matrix< LocalFloatType, 6, 1 > waist_speed, waist_acc ;
//  Eigen::Matrix< LocalFloatType, 3, 1 > waist_theta ;
//  // compute the speed and acceleration of the waist in the world frame
//  if (PreviousSupportFoot_)
//    {
//      Jac_LF::run(robot_, prev_q_, Vector3dTpl<LocalFloatType>::Type(0,0,0), jacobian_lf_);
//      waist_speed = jacobian_lf_ * prev_dq_ ;
//      waist_acc = jacobian_lf_ * prev_ddq_ /* + d_jacobian_lf_ * prev_dq_*/ ;
//    }else
//    {
//      Jac_RF::run(robot_, prev_q_, Vector3dTpl<LocalFloatType>::Type(0,0,0), jacobian_rf_);
//      waist_speed = jacobian_rf_ * prev_dq_ ;
//      waist_acc = jacobian_rf_ * prev_ddq_ /*+ d_jacobian_rf_ * prev_dq_*/ ;
//    }
//  for (unsigned int i = 0 ; i < 6 ; ++i)
//    {
//      dq_(i,0)   = waist_speed(i,0);
//      ddq_(i,0)  = waist_acc(i,0);
//    }
//  // compute the position of the waist in the world frame
//  RootNode & node_waist = boost::fusion::at_c<Robot_Model::BODY>(robot_.nodes);
//  waist_theta(0,0) = prev_q_(3,0) ;
//  waist_theta(1,0) = prev_dq_(4,0) ;
//  waist_theta(2,0) = prev_ddq_(5,0) ;
//  q_(0,0) = node_waist.body.iX0.inverse().r()(0,0) ;
//  q_(1,0) = node_waist.body.iX0.inverse().r()(1,0) ;
//  q_(2,0) = node_waist.body.iX0.inverse().r()(2,0) ;
//  q_(3,0) = waist_theta(0,0) ;
//  q_(4,0) = waist_theta(1,0) ;
//  q_(5,0) = waist_theta(2,0) ;

//  if (inputLeftFoot.stepType<0)
//    {
//      PreviousSupportFoot_ = true ; // left foot is supporting
//    }
//  else
//    {
//      PreviousSupportFoot_ = false ; // right foot is supporting
//    }
//  prev_q_ = q_ ;
//  prev_dq_ = dq_ ;
//  prev_ddq_ = ddq_ ;

  //  Robot_Model::confVector q, dq, ddq;
  //  for(unsigned int j = 0 ; j < 6 ; j++ )
  //  {
  //    q(j,0) = 0 ;
  //    dq(j,0) = 0 ;
  //    ddq(j,0) = 0 ;
  //  }
  //  for(unsigned int j = 6 ; j < ZMPMBConfiguration_.size() ; j++ )
  //  {
  //    q(j,0) = ZMPMBConfiguration_(j) ;
  //    dq(j,0) = ZMPMBVelocity_(j) ;
  //    ddq(j,0) = ZMPMBAcceleration_(j) ;
  //  }
  //
  //  metapod::rnea< Robot_Model, true >::run(robot_2, q, dq, ddq);
  //  LankleNode & node_lankle = boost::fusion::at_c<Robot_Model::l_ankle>(robot_2.nodes);
  //  RankleNode & node_rankle = boost::fusion::at_c<Robot_Model::r_ankle>(robot_2.nodes);
  //
  //  CWx = node_waist.body.iX0.r()(0,0) - inputCoMState.x[0] ;
  //  CWy = node_waist.body.iX0.r()(1,0) - inputCoMState.y[0] ;
  //
  //  // Debug Purpose
  //  // -------------
  //  ofstream aof;
  //  string aFileName;
  //  ostringstream oss(std::ostringstream::ate);
  //  static int it = 0;
  //
  //  // --------------------
  //  oss.str("DynamicFilterMetapodAccWaistSupportFoot.dat");
  //  aFileName = oss.str();
  //  if(it == 0)
  //  {
  //    aof.open(aFileName.c_str(),ofstream::out);
  //    aof.close();
  //  }
  //  ///----
  //  aof.open(aFileName.c_str(),ofstream::app);
  //  aof.precision(8);
  //  aof.setf(ios::scientific, ios::floatfield);
  //  aof << filterprecision( it*samplingPeriod) << " " ;     // 1
  //
  //  if (inputLeftFoot.stepType < 0)
  //  {
  //    aof << filterprecision( node_lankle.body.ai.v()(0,0) ) << " "  // 2
  //        << filterprecision( node_lankle.body.ai.v()(1,0) ) << " "  // 3
  //        << filterprecision( node_lankle.body.ai.v()(2,0) ) << " "  // 4
  //        << filterprecision( node_lankle.body.ai.w()(0,0) ) << " "  // 5
  //        << filterprecision( node_lankle.body.ai.w()(1,0) ) << " "  // 6
  //        << filterprecision( node_lankle.body.ai.w()(2,0) ) << " "; // 7
  //  }else
  //  {
  //    aof << filterprecision( node_rankle.body.ai.v()(0,0) ) << " "  // 2
  //        << filterprecision( node_rankle.body.ai.v()(1,0) ) << " "  // 3
  //        << filterprecision( node_rankle.body.ai.v()(2,0) ) << " "  // 4
  //        << filterprecision( node_rankle.body.ai.w()(0,0) ) << " "  // 5
  //        << filterprecision( node_rankle.body.ai.w()(1,0) ) << " "  // 6
  //        << filterprecision( node_rankle.body.ai.w()(2,0) ) << " " ;// 7
  //  }
  //
  //  aof << filterprecision( inputCoMState.x[2] ) << " "           // 8
  //      << filterprecision( inputCoMState.y[2] ) << " "           // 9
  //      << filterprecision( inputCoMState.z[2] ) << " "           // 10
  //      << filterprecision( inputCoMState.roll[2] ) << " "        // 11
  //      << filterprecision( inputCoMState.pitch[2] ) << " "       // 12
  //      << filterprecision( inputCoMState.yaw[2] ) << " "       ; // 13
  //
  //  if (inputLeftFoot.stepType < 0)
  //  {
  //    aof << filterprecision( node_lankle.body.vi.v()(0,0) ) << " "  // 14
  //        << filterprecision( node_lankle.body.vi.v()(1,0) ) << " "  // 15
  //        << filterprecision( node_lankle.body.vi.v()(2,0) ) << " "  // 16
  //        << filterprecision( node_lankle.body.vi.w()(0,0) ) << " "  // 17
  //        << filterprecision( node_lankle.body.vi.w()(1,0) ) << " "  // 18
  //        << filterprecision( node_lankle.body.vi.w()(2,0) ) << " " ;// 19
  //  }else
  //  {
  //    aof << filterprecision( node_rankle.body.vi.v()(0,0) ) << " "  // 14
  //        << filterprecision( node_rankle.body.vi.v()(1,0) ) << " "  // 15
  //        << filterprecision( node_rankle.body.vi.v()(2,0) ) << " "  // 16
  //        << filterprecision( node_rankle.body.vi.w()(0,0) ) << " "  // 17
  //        << filterprecision( node_rankle.body.vi.w()(1,0) ) << " "  // 18
  //        << filterprecision( node_rankle.body.vi.w()(2,0) ) << " "; // 19
  //  }
  //
  //  aof << filterprecision( inputCoMState.x[1] ) << " "           // 20
  //      << filterprecision( inputCoMState.y[1] ) << " "           // 21
  //      << filterprecision( inputCoMState.z[1] ) << " "           // 22
  //      << filterprecision( inputCoMState.roll[1] ) << " "        // 23
  //      << filterprecision( inputCoMState.pitch[1] ) << " "       // 24
  //      << filterprecision( inputCoMState.yaw[1] ) << " "     ;   // 25
  //
  //  aof << filterprecision( node_waist.joint.vj.v()(0,0) ) << " " // 26
  //      << filterprecision( node_waist.joint.vj.v()(1,0) ) << " "  // 27
  //      << filterprecision( node_waist.joint.vj.v()(2,0) ) << " "  // 28
  //      << filterprecision( node_waist.joint.vj.w()(0,0) ) << " "  // 29
  //      << filterprecision( node_waist.joint.vj.w()(1,0) ) << " "  // 30
  //      << filterprecision( node_waist.joint.vj.w()(2,0) ) << " "; // 31
  //
  //  aof << filterprecision( inputCoMState.x[0] ) << " "           // 32
  //      << filterprecision( inputCoMState.y[0] ) << " "           // 33
  //      << filterprecision( inputCoMState.z[0] ) << " "           // 34
  //      << filterprecision( inputCoMState.roll[0] ) << " "        // 35
  //      << filterprecision( inputCoMState.pitch[0] ) << " "       // 36
  //      << filterprecision( inputCoMState.yaw[0] ) << " "     ;   // 37
  //
  //  aof << filterprecision( node_waist.body.iX0.r()(0,0) ) << " " // 38
  //      << filterprecision( node_waist.body.iX0.r()(1,0) ) << " " // 39
  //      << filterprecision( node_waist.body.iX0.r()(2,0) ) << " ";// 40
  //
  //
  //  for(unsigned int j = 0 ; j < ZMPMBConfiguration_.size() ; j++ )//41
  //    aof << filterprecision( ZMPMBConfiguration_(j) ) << " " ;
  //  for(unsigned int j = 0 ; j < ZMPMBVelocity_.size() ; j++ )    //77
  //    aof << filterprecision( ZMPMBVelocity_(j) ) << " " ;
  //  for(unsigned int j = 0 ; j < ZMPMBAcceleration_.size() ; j++ )//113
  //    aof << filterprecision( ZMPMBAcceleration_(j) ) << " " ;
  //
  //
  //  aof << endl ;
  //  aof.close();
  //  ++it;

  return ;
}

void DynamicFilter::Debug(const deque<COMState> & ctrlCoMState,
                          const deque<FootAbsolutePosition> & ctrlLeftFoot,
                          const deque<FootAbsolutePosition> & ctrlRightFoot,
                          const deque<COMState> & inputCOMTraj_deq_,
                          const deque<ZMPPosition> inputZMPTraj_deq_,
                          const deque<FootAbsolutePosition> & inputLeftFootTraj_deq_,
                          const deque<FootAbsolutePosition> & inputRightFootTraj_deq_,
                          const deque<COMState> & outputDeltaCOMTraj_deq_)
{
  deque<COMState> CoM_tmp = ctrlCoMState ;
  for (unsigned int i = 0 ; i < NCtrl_ ; ++i)
  {
    for(int j=0;j<3;j++)
    {
      CoM_tmp[i].x[j] += outputDeltaCOMTraj_deq_[i].x[j] ;
      CoM_tmp[i].y[j] += outputDeltaCOMTraj_deq_[i].y[j] ;
    }
  }

  vector< vector<double> > zmpmb_corr (NCtrl_,vector<double>(2,0.0));
  for(unsigned int i = 0 ; i < NCtrl_ ; ++i)
  {
      InverseKinematics( CoM_tmp[i], ctrlLeftFoot[i], ctrlRightFoot[i],
                         ZMPMBConfiguration_, ZMPMBVelocity_, ZMPMBAcceleration_,
                         controlPeriod_, 2, 20) ;

      InverseDynamics(ZMPMBConfiguration_, ZMPMBVelocity_, ZMPMBAcceleration_);

      ExtractZMP(zmpmb_corr[i]);
  }

  int inc = (int)round(interpolationPeriod_/controlPeriod_) ;
  ofstream aof;
  string aFileName;
  static int iteration_zmp = 0 ;
  ostringstream oss(std::ostringstream::ate);
  oss.str("zmpmb_herdt.txt");
  aFileName = oss.str();
  if ( iteration_zmp == 0 )
  {
    aof.open(aFileName.c_str(),ofstream::out);
    aof.close();
  }

  aof.open(aFileName.c_str(),ofstream::app);
  aof.precision(8);
  aof.setf(ios::scientific, ios::floatfield);
  for (unsigned int i = 0 ; i < NbI_ ; ++i)
  {
    aof << inputZMPTraj_deq_[i*inc].px << " " ;       // 1
    aof << inputZMPTraj_deq_[i*inc].py << " " ;       // 2

    aof << ZMPMB_vec_[i][0] << " " ;                  // 3
    aof << ZMPMB_vec_[i][1] << " " ;                  // 4

    aof << inputCOMTraj_deq_[i].x[0] << " " ;         // 5
    aof << inputCOMTraj_deq_[i].x[1] << " " ;         // 6
    aof << inputCOMTraj_deq_[i].x[2] << " " ;         // 7

    aof << inputLeftFootTraj_deq_[i].x << " " ;       // 8
    aof << inputLeftFootTraj_deq_[i].dx << " " ;      // 9
    aof << inputLeftFootTraj_deq_[i].ddx << " " ;     // 10

    aof << inputRightFootTraj_deq_[i].x << " " ;      // 11
    aof << inputRightFootTraj_deq_[i].dx << " " ;     // 12
    aof << inputRightFootTraj_deq_[i].ddx << " " ;    // 13

    aof << inputCOMTraj_deq_[i].y[0] << " " ;         // 14
    aof << inputCOMTraj_deq_[i].y[1] << " " ;         // 15
    aof << inputCOMTraj_deq_[i].y[2] << " " ;         // 16

    aof << inputLeftFootTraj_deq_[i].y << " " ;       // 17
    aof << inputLeftFootTraj_deq_[i].dy << " " ;      // 18
    aof << inputLeftFootTraj_deq_[i].ddy << " " ;     // 19

    aof << inputRightFootTraj_deq_[i].y << " " ;      // 20
    aof << inputRightFootTraj_deq_[i].dy << " " ;     // 21
    aof << inputRightFootTraj_deq_[i].ddy << " " ;    // 22

    aof << inputCOMTraj_deq_[i].yaw[0] << " " ;       // 23
    aof << inputCOMTraj_deq_[i].yaw[1] << " " ;       // 24
    aof << inputCOMTraj_deq_[i].yaw[2] << " " ;       // 25

    aof << inputLeftFootTraj_deq_[i].theta << " " ;   // 26
    aof << inputLeftFootTraj_deq_[i].dtheta << " " ;  // 27
    aof << inputLeftFootTraj_deq_[i].ddtheta << " " ; // 28

    aof << inputRightFootTraj_deq_[i].theta << " " ;  // 29
    aof << inputRightFootTraj_deq_[i].dtheta << " " ; // 30
    aof << inputRightFootTraj_deq_[i].ddtheta << " " ;// 31

    aof << inputCOMTraj_deq_[i].z[0] << " " ;         // 32
    aof << inputCOMTraj_deq_[i].z[1] << " " ;         // 33
    aof << inputCOMTraj_deq_[i].z[2] << " " ;         // 34

    aof << inputLeftFootTraj_deq_[i].z << " " ;       // 35
    aof << inputLeftFootTraj_deq_[i].dz << " " ;      // 36
    aof << inputLeftFootTraj_deq_[i].ddz << " " ;     // 37

    aof << inputRightFootTraj_deq_[i].z << " " ;      // 38
    aof << inputRightFootTraj_deq_[i].dz << " " ;     // 39
    aof << inputRightFootTraj_deq_[i].ddz << " " ;    // 40

    aof << CoM_tmp[i*(int)round(interpolationPeriod_/controlPeriod_)].x[0] << " " ; // 41
    aof << CoM_tmp[i*(int)round(interpolationPeriod_/controlPeriod_)].x[1] << " " ; // 42
    aof << CoM_tmp[i*(int)round(interpolationPeriod_/controlPeriod_)].x[2] << " " ; // 43

    aof << CoM_tmp[i*(int)round(interpolationPeriod_/controlPeriod_)].y[0] << " " ; // 44
    aof << CoM_tmp[i*(int)round(interpolationPeriod_/controlPeriod_)].y[1] << " " ; // 45
    aof << CoM_tmp[i*(int)round(interpolationPeriod_/controlPeriod_)].y[2] << " " ; // 46

    aof << endl ;
  }
  aof.close();

  aFileName = "zmpmb_corr_herdt.txt" ;
  if ( iteration_zmp == 0 )
  {
    aof.open(aFileName.c_str(),ofstream::out);
    aof.close();
  }
  aof.open(aFileName.c_str(),ofstream::app);
  aof.precision(8);
  aof.setf(ios::scientific, ios::floatfield);
  for (unsigned int i = 0 ; i < NCtrl_ ; ++i)
  {
    aof << zmpmb_corr[i][0] << " " ;
    aof << zmpmb_corr[i][1] << " " ;
    aof << outputDeltaCOMTraj_deq_[i].x[0] << " "  ;
    aof << outputDeltaCOMTraj_deq_[i].y[0] << " "  ;
    aof << outputDeltaCOMTraj_deq_[i].x[1] << " "  ;
    aof << outputDeltaCOMTraj_deq_[i].y[1] << " "  ;
    aof << outputDeltaCOMTraj_deq_[i].x[2] << " "  ;
    aof << outputDeltaCOMTraj_deq_[i].y[2] << " "  ;
    aof << deltaZMPx_[i] << " " ;
    aof << deltaZMPy_[i] << " " ;
    aof << sxzmp_[i] << " " ;
    aof << syzmp_[i] << " " ;
    aof << deltaZMP_deq_[i].px << " " ;
    aof << deltaZMP_deq_[i].py << " " ;
    aof << endl ;
  }
  aof.close();

  oss.str("/tmp/buffer_");
  oss << setfill('0') << setw(3) << iteration_zmp << ".txt" ;
  aFileName = oss.str();
  aof.open(aFileName.c_str(),ofstream::out);
  aof.close();

  aof.open(aFileName.c_str(),ofstream::app);
  aof.precision(8);
  aof.setf(ios::scientific, ios::floatfield);
  for (unsigned int i = 0 ; i < NCtrl_*(PG_N_-1) ; ++i)
  {
    aof << i << " " ; // 0
    aof << inputZMPTraj_deq_[i].px << " " ;           // 1
    aof << inputZMPTraj_deq_[i].py << " " ;           // 2

    aof << zmpmb_i_[i][0] << " " ;                    // 3
    aof << zmpmb_i_[i][1] << " " ;                    // 4

    aof << ctrlCoMState[i].x[0] << " " ;         // 5
    aof << ctrlCoMState[i].x[1] << " " ;         // 6
    aof << ctrlCoMState[i].x[2] << " " ;         // 7

    aof << ctrlLeftFoot[i].x << " " ;       // 8
    aof << ctrlLeftFoot[i].dx << " " ;      // 9
    aof << ctrlLeftFoot[i].ddx << " " ;     // 10

    aof << ctrlRightFoot[i].x << " " ;      // 11
    aof << ctrlRightFoot[i].dx << " " ;     // 12
    aof << ctrlRightFoot[i].ddx << " " ;    // 13

    aof << ctrlCoMState[i].y[0] << " " ;         // 14
    aof << ctrlCoMState[i].y[1] << " " ;         // 15
    aof << ctrlCoMState[i].y[2] << " " ;         // 16

    aof << ctrlLeftFoot[i].y << " " ;       // 17
    aof << ctrlLeftFoot[i].dy << " " ;      // 18
    aof << ctrlLeftFoot[i].ddy << " " ;     // 19

    aof << ctrlRightFoot[i].y << " " ;      // 20
    aof << ctrlRightFoot[i].dy << " " ;     // 21
    aof << ctrlRightFoot[i].ddy << " " ;    // 22

    aof << ctrlCoMState[i].yaw[0] << " " ;       // 23
    aof << ctrlCoMState[i].yaw[1] << " " ;       // 24
    aof << ctrlCoMState[i].yaw[2] << " " ;       // 25

    aof << ctrlLeftFoot[i].theta << " " ;   // 26
    aof << ctrlLeftFoot[i].dtheta << " " ;  // 27
    aof << ctrlLeftFoot[i].ddtheta << " " ; // 28

    aof << ctrlRightFoot[i].theta << " " ;  // 29
    aof << ctrlRightFoot[i].dtheta << " " ; // 30
    aof << ctrlRightFoot[i].ddtheta << " " ;// 31

    aof << ctrlCoMState[i].z[0] << " " ;         // 32
    aof << ctrlCoMState[i].z[1] << " " ;         // 33
    aof << ctrlCoMState[i].z[2] << " " ;         // 34

    aof << ctrlLeftFoot[i].z << " " ;       // 35
    aof << ctrlLeftFoot[i].dz << " " ;      // 36
    aof << ctrlLeftFoot[i].ddz << " " ;     // 37

    aof << ctrlRightFoot[i].z << " " ;      // 38
    aof << ctrlRightFoot[i].dz << " " ;     // 39
    aof << ctrlRightFoot[i].ddz << " " ;    // 40

    aof << deltaZMP_deq_[i].px << " " ;    // 41
    aof << deltaZMP_deq_[i].py << " " ;    // 42


    aof << endl ;
  }
  aof.close();
  iteration_zmp++;
  return ;
}
