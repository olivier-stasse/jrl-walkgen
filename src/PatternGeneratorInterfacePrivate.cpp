/* \doc This object is the interface to the walking gait
   generation rchitecture
   Copyright (c) 2005-2009, 
   Olivier Stasse,

   JRL-Japan, CNRS/AIST

   All rights reserved.
   
   Please look at License.txt for details on the license.

*/
#include <fstream>
#include <time.h>
	
#ifdef UNIX
#include <sys/time.h>
#endif /*UNIX*/

#ifdef WIN32
#include <Windows.h>
#include <TimeUtilsWindows.h>
#define bzero(p, size) (void)memset((p), 0, (size))  // definition of bzero for win32
#endif /*WIN32*/


#include <PatternGeneratorInterfacePrivate.h>

#include <Debug.h>

namespace PatternGeneratorJRL {

  PatternGeneratorInterfacePrivate::PatternGeneratorInterfacePrivate(CjrlHumanoidDynamicRobot *aHDR)
    : PatternGeneratorInterface(aHDR),SimplePlugin(this)
  {
    m_HumanoidDynamicRobot = aHDR;

    ODEBUG4("Step 0","DebugPGI.txt");

    // Initialization for debugging.
    RESETDEBUG4("DebugDataWPDisplay.txt");

    RESETDEBUG4("DebugDataqrDisplay.txt");
    RESETDEBUG4("DebugDataqlDisplay.txt");
    RESETDEBUG4("DebugDataZMPMB1Display.txt");

    RESETDEBUG4("DebugDataCOMForHeuristic.txt");
    RESETDEBUG4("DebugDataqArmsHeuristic.txt");

    RESETDEBUG4("DebugDatadqrDisplay.txt");
    RESETDEBUG4("DebugDatadqlDisplay.txt");
    RESETDEBUG4("DebugDataUBDisplay.txt");
    RESETDEBUG4("DebugDatadUBDisplay.txt");

    RESETDEBUG4("DebugData.txt");
    RESETDEBUG4("DebugPGI.txt");
    RESETDEBUG5("DebugDataLong.txt");
    RESETDEBUG4("DebugDataZMPTargetZ.dat");
    RESETDEBUG4("LeftLegAngle.txt");
    RESETDEBUG4("RightLegAngle.txt");
    ODEBUG4("Step 1","DebugPGI.txt");
    RESETDEBUG4("DebugDataIKArms.txt");
    RESETDEBUG4("DebugDataOnLine.txt");
    RESETDEBUG4("DebugDataWaistYaw.dat");
    RESETDEBUG4("DebugDataAngularMomentum.dat");

    RESETDEBUG4("UpperBodyAngles.txt");
    RESETDEBUG4("DebugZMPFinale.txt");

    RESETDEBUG4("DebugGMFKW.dat");
    RESETDEBUG4("DebugDataCOM.txt");

    RESETDEBUG4("DDCC.dat");
    RESETDEBUG4("DDCV.dat");
    RESETDEBUG4("DebugDataWaist.dat");
    // End if Initialization

    m_ObstacleDetected = false;
    m_AutoFirstStep = false;

    // Initialization of obstacle parameters informations.
    m_ObstaclePars.x=1.0;
    m_ObstaclePars.y=0.0;
    m_ObstaclePars.z=0.0;
    m_ObstaclePars.theta=0.0;
    m_ObstaclePars.h=0.05;
    m_ObstaclePars.w=1.0;
    m_ObstaclePars.d=0.05;

    m_TimeDistrFactor.resize(4);
    m_TimeDistrFactor[0]=2.0;
    m_TimeDistrFactor[1]=3.7;
    m_TimeDistrFactor[2]=1.0;
    m_TimeDistrFactor[3]=3.0;

    m_DeltaFeasibilityLimit =0.0;

    ODEBUG("Beginning of Object Instanciation ");
    // Initialization of the fundamental objects.
    ObjectsInstanciation();
    ODEBUG("End of Object Instanciation ");
    // Initialize their relationships.

    ODEBUG("Beginning of Inter Object Relation Initialization " );
    InterObjectRelationInitialization();
    ODEBUG("End of Inter Object Relation Initialization ");
    // Initialization of the strategy for ZMP ref trajectory generation.
    m_AlgorithmforZMPCOM = ZMPCOM_KAJITA_2003;

    // Initialize (if needed) debugging actions.
    m_dt = 0.005;
    m_DOF = m_HumanoidDynamicRobot->numberDof();

    m_SamplingPeriod = m_PC->SamplingPeriod();
    m_PreviewControlTime = m_PC->PreviewControlTime();
    m_NL = (unsigned int)(m_PreviewControlTime/m_SamplingPeriod);

    /* For debug purposes. */
    MAL_VECTOR_RESIZE(m_Debug_prev_qr,6);
    MAL_VECTOR_RESIZE(m_Debug_prev_dqr,6);
    MAL_VECTOR_RESIZE(m_Debug_prev_ql,6);
    MAL_VECTOR_RESIZE(m_Debug_prev_dql,6);

    MAL_VECTOR_RESIZE(m_Debug_prev_qr_RefState,6);
    MAL_VECTOR_RESIZE(m_Debug_prev_ql_RefState,6);

    MAL_VECTOR_RESIZE(m_Debug_prev_UpperBodyAngles,28);

    m_ZMPShift.resize(4);
    m_ZMPShift[0] = 0.02;
    m_ZMPShift[1] = 0.07;
    m_ZMPShift[2] = 0.02;
    m_ZMPShift[3] = 0.02;

    for(int i=0;i<4;i++)
      {
	for(int j=0;j<4;j++)
	  if (i==j)
	    MAL_S4x4_MATRIX_ACCESS_I_J(m_MotionAbsPos, i,j) =
	      MAL_S4x4_MATRIX_ACCESS_I_J(m_MotionAbsOrientation, i,j) =
	      MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistAbsPos, i,j) =
	      MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistRelativePos, i,j) =
	      1.0;
	  else
	    MAL_S4x4_MATRIX_ACCESS_I_J(m_MotionAbsPos, i,j) =
	      MAL_S4x4_MATRIX_ACCESS_I_J(m_MotionAbsOrientation, i,j) =
	      MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistAbsPos, i,j) =
	      MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistRelativePos, i,j) =
	      0.0;

      }


    ODEBUG4("Step 2","DebugPGI.txt");

    //  string PCParameters="/home/stasse/OpenHRP/PatternGeneratorJRL/src/PreviewControlParameters.ini";

    ofstream DebugFile;
    ofstream DebugFileLong;
    ofstream DebugFileUpperBody;
    m_count = 0;

    ODEBUG4("Step 3","DebugPGI.txt");

    ODEBUG4("Step 4","DebugPGI.txt");

    m_TSsupport = 0.78;
    m_TDsupport = 0.02;

    m_FirstPrint = true;
    m_FirstRead = true;
    ODEBUG4("Step 5","DebugPGI.txt");

    m_NewStepX = 0.0;
    m_NewStepY = 0.0;
    m_NewTheta = 0.0;
    m_NewStep = false;
    m_ShouldBeRunning = false;

    m_AbsMotionTheta = 0;
    m_InternalClock = 0.0;

    for(unsigned int i=0;i<3;i++)
      m_ZMPInitialPoint(i)=0.0;
    m_ZMPInitialPointSet = false;

    RegisterPluginMethods();
  }

  void PatternGeneratorInterfacePrivate::RegisterPluginMethods()
  {
    std::string aMethodName[11] = 
      {":LimitsFeasibility",
       ":ZMPShiftParameters",
       ":TimeDistributionParameters",
       ":stepseq",
       ":finish",
       ":StartOnLineStepSequencing",
       ":StopOnLineStepSequencing",
       ":readfilefromkw",
       ":SetAlgoForZmpTrajectory",
       ":SetAutoFirstStep",
       ":ChangeNextStep"};
    
    for(int i=0;i<11;i++)
      {
	if (!SimplePlugin::RegisterMethod(aMethodName[i]))
	  {
	    std::cerr << "Unable to register " << aMethodName << std::endl;
	  }
	else
	  {
	    ODEBUG("Succeed in registering " << aMethodName[i]);
	  }
      }

  }
  void PatternGeneratorInterfacePrivate::ObjectsInstanciation()
  {
    // Create fundamental objects to make the WPG runs.
    {
      string inProperty[2]={"ComputeZMP",
			    "ComputeBackwardDynamics"};
      string inValue[2]={"true","false"};
      for(unsigned int i=0;i<2;i++)
	m_HumanoidDynamicRobot->setProperty(inProperty[i],inValue[i]);
      
    }

    // INFO: This where you should instanciate your own
    // INFO: object for Com and Foot realization.
    // INFO: The default one is based on a geometrical approach.
    m_ComAndFootRealization = new ComAndFootRealizationByGeometry(this);

    // Creates the foot trajectory generator.
    m_FeetTrajectoryGenerator = new LeftAndRightFootTrajectoryGenerationMultiple(this,
										 m_HumanoidDynamicRobot->leftFoot());

    // ZMP reference and Foot trajectory planner (Preview control method from Kajita2003)
    m_ZMPD = new ZMPDiscretization(this,"",m_HumanoidDynamicRobot);

    // ZMP and CoM generation using the method proposed in Wieber2006.
    m_ZMPQP = new ZMPQPWithConstraint(this,"",m_HumanoidDynamicRobot);

    // ZMP and CoM generation using the method proposed in Dimitrov2008.
    m_ZMPCQPFF = new ZMPConstrainedQPFastFormulation(this,"",m_HumanoidDynamicRobot);

    // ZMP and CoM generation using the analytical method proposed in Morisawa2007.
    m_ZMPM = new AnalyticalMorisawaCompact(this);
    m_ZMPM->SetHumanoidSpecificities(m_HumanoidDynamicRobot);

    // Preview control for a 3D Linear inverse pendulum
    m_PC = new PreviewControl(this);

    // Object to generate Motion from KineoWorks.
    m_GMFKW = new GenerateMotionFromKineoWorks();

    // Object to have a Dynamic multibody robot model.
    // for the second preview loop.


    // Stack of steps handler.
    m_StepStackHandler = new StepStackHandler(this);

    // Stepping over planner.
    m_StOvPl = new StepOverPlanner(m_ObstaclePars,
				   m_HumanoidDynamicRobot);


    // The creation of the double stage preview control manager.
    m_DoubleStagePCStrategy = new DoubleStagePreviewControlStrategy(this);
    m_DoubleStagePCStrategy->SetBufferPositions(&m_ZMPPositions,
						&m_COMBuffer,
						&m_LeftFootPositions,
						&m_RightFootPositions);

    
    m_CoMAndFootOnlyStrategy = new CoMAndFootOnlyStrategy(this);
    m_CoMAndFootOnlyStrategy->SetBufferPositions(&m_ZMPPositions,
						 &m_COMBuffer,
						 &m_LeftFootPositions,
						 &m_RightFootPositions);

    // Defa`<ult handler :DoubleStagePreviewControl.
    m_GlobalStrategyManager = m_DoubleStagePCStrategy;

    // End of the creation of the fundamental objects.

  }

  void PatternGeneratorInterfacePrivate::InterObjectRelationInitialization()
  {
    // Initialize the Preview Control.
    m_Zc = m_PC->GetHeightOfCoM();

    // Initialize the Preview Control general object.
    m_DoubleStagePCStrategy->InitInterObjects(m_PC,
					      m_HumanoidDynamicRobot,
					      m_ComAndFootRealization,
					      m_StepStackHandler);
        
    m_CoMAndFootOnlyStrategy->InitInterObjects(m_PC,
					       m_HumanoidDynamicRobot,
					       m_ComAndFootRealization,
					       m_StepStackHandler);

    // Initialize the ZMP trajectory generator.
    m_ZMPD->SetSamplingPeriod(m_PC->SamplingPeriod());
    m_ZMPD->SetTimeWindowPreviewControl(m_PC->PreviewControlTime());
    m_ZMPD->SetPreviewControl(m_PC);

    m_ZMPQP->SetSamplingPeriod(m_PC->SamplingPeriod());
    m_ZMPQP->SetTimeWindowPreviewControl(m_PC->PreviewControlTime());
    m_ZMPQP->SetPreviewControl(m_PC);

    m_ZMPCQPFF->SetSamplingPeriod(m_PC->SamplingPeriod());
    m_ZMPCQPFF->SetTimeWindowPreviewControl(m_PC->PreviewControlTime());
    m_ZMPCQPFF->SetPreviewControl(m_PC);
    
    m_ZMPM->SetSamplingPeriod(m_PC->SamplingPeriod());
    m_ZMPM->SetTimeWindowPreviewControl(m_PC->PreviewControlTime());
    m_ZMPM->SetFeetTrajectoryGenerator(m_FeetTrajectoryGenerator);

    // The motion generator based on a Kineoworks pathway is given here.
    m_GMFKW->SetPreviewControl(m_PC);

    // Read the robot VRML file model.
    
    m_ComAndFootRealization->setHumanoidDynamicRobot(m_HumanoidDynamicRobot);

    m_ComAndFootRealization->SetHeightOfTheCoM(m_PC->GetHeightOfCoM());

    m_ComAndFootRealization->setSamplingPeriod(m_PC->SamplingPeriod());

    m_ComAndFootRealization->SetStepStackHandler(m_StepStackHandler);

    m_ComAndFootRealization->Initialization();

    m_StOvPl->SetPreviewControl(m_PC);
    m_StOvPl->SetDynamicMultiBodyModel(m_HumanoidDynamicRobot);
    m_StOvPl->SetZMPDiscretization(m_ZMPD);


    m_StepStackHandler->SetStepOverPlanner(m_StOvPl);
    m_StepStackHandler->SetWalkMode(0);
    // End of the initialization of the fundamental object. 
  }

  PatternGeneratorInterfacePrivate::~PatternGeneratorInterfacePrivate()
  {


    ODEBUG4("Destructor: Start","DebugPGI.txt");

    if (m_StOvPl!=0)
      delete m_StOvPl;
    ODEBUG4("Destructor: did m_StOvPl","DebugPGI.txt");

    if (m_StepStackHandler!=0)
      delete m_StepStackHandler;
    ODEBUG4("Destructor: did m_StepStackHandler","DebugPGI.txt");



    if (m_HumanoidDynamicRobot!=0)
      delete m_HumanoidDynamicRobot;
    ODEBUG4("Destructor: did m_HumanoidDynamicRobot","DebugPGI.txt");

    if (m_GMFKW!=0)
      delete m_GMFKW;
    ODEBUG4("Destructor: did m_GMKFW","DebugPGI.txt");

    if (m_PC!=0)
      delete m_PC;
    ODEBUG4("Destructor: did m_PC","DebugPGI.txt");

    if (m_ZMPD!=0)
      delete m_ZMPD;
    ODEBUG4("Destructor: did m_ZMPD","DebugPGI.txt");

    if (m_ZMPQP!=0)
      delete m_ZMPQP;

    if (m_ZMPCQPFF!=0)
      delete m_ZMPCQPFF;
    ODEBUG4("Destructor: did m_ZMPQP","DebugPGI.txt");

    if (m_ZMPM!=0)
      delete m_ZMPM;
    ODEBUG4("Destructor: did m_ZMPM","DebugPGI.txt");

    if (m_ComAndFootRealization!=0)
      delete m_ComAndFootRealization;

    if (m_FeetTrajectoryGenerator!=0)
      delete m_FeetTrajectoryGenerator;

    if (m_DoubleStagePCStrategy!=0)
      delete m_DoubleStagePCStrategy;

    if (m_CoMAndFootOnlyStrategy!=0)
      delete m_CoMAndFootOnlyStrategy;
  }


  void PatternGeneratorInterfacePrivate::m_SetZMPShiftParameters(istringstream &strm)
  {
    ODEBUG("SetZMPShitParameters");
    while(!strm.eof())
      {
	if (!strm.eof())
	  {
	    strm >> m_ZMPShift[0];

	  }
	else break;
	if (!strm.eof())
	  {
	    strm >> m_ZMPShift[1];

	  }
	else break;
	if (!strm.eof())
	  {
	    strm >> m_ZMPShift[2];

	  }
	else break;
	if (!strm.eof())
	  {
	    strm >> m_ZMPShift[3];

	  }
	else break;
      }
  }


  void PatternGeneratorInterfacePrivate::m_SetLimitsFeasibility(istringstream &strm)
  {
    while(!strm.eof())
      {
	if (!strm.eof())
	  {
	    // TODO : forward this to ComAndFootRealization.
	    strm >> m_DeltaFeasibilityLimit;
	  }
	else break;
      }
  }
  
  void PatternGeneratorInterfacePrivate::ReadSequenceOfSteps(istringstream &strm)
  {
    // Read the data inside strm.
    

    switch (m_StepStackHandler->GetWalkMode())
      {
      case 0:
      case 4:
      case 3:
      case 1:
	{
	  ODEBUG("Juste before the reading of the step sequence ");
	  m_StepStackHandler->ReadStepSequenceAccordingToWalkMode(strm);
	  break;
	}
      case 2:
	{
	  ODEBUG( "Walk Mode with Obstacle StepOver Selected \
                 (obstacle parameters have to be set first, \
                 if not standard dimensions are used)" );
	  m_StOvPl->SetObstacleInformation(m_ObstaclePars);
	  m_StOvPl->SetDeltaStepOverCOMHeightMax(m_DeltaFeasibilityLimit);
	  //cout << "I am calculating relative positions to negociate obstacle" << endl;

	  // Update stack of relative foot by using StpOvPl.
	  m_StepStackHandler->ReadStepSequenceAccordingToWalkMode(strm);

	  break;
	}
      default:
	{
	  ODEBUG3( "Please select proper walk mode. \
            (0 for normal walking ; \
             1 for walking with waistheight variation ; \
             2 for walking with obstacle stepover)" );
	  return;
	}
      }
    ODEBUG("Just before starting to Finish and RealizeStepSequence()");

  }

  void PatternGeneratorInterfacePrivate::m_StepSequence(istringstream &strm)
  {

    ODEBUG("Step Sequence");
    ofstream DebugFile;
    ReadSequenceOfSteps(strm);
    ODEBUG("After reading Step Sequence");
    FinishAndRealizeStepSequence();
    ODEBUG("After finish and realize Step Sequence");
  }

  void PatternGeneratorInterfacePrivate::EvaluateStartingCOM(MAL_VECTOR(  & Configuration,double),
						      MAL_S3_VECTOR(  & lStartingCOMPosition,double))
  {
    MAL_VECTOR(Velocity,double);
    MAL_VECTOR_RESIZE(Velocity,MAL_VECTOR_SIZE(Configuration));
    for(unsigned int i=0;i<MAL_VECTOR_SIZE(Configuration);i++)
      Velocity[i] = 0.0;

    m_HumanoidDynamicRobot->currentConfiguration(Configuration);
    m_HumanoidDynamicRobot->currentVelocity(Velocity);
    m_HumanoidDynamicRobot->computeForwardKinematics();
    lStartingCOMPosition = m_HumanoidDynamicRobot->positionCenterOfMass();
  }

  void PatternGeneratorInterfacePrivate::EvaluateStartingState(COMPosition  & lStartingCOMPosition,
							MAL_S3_VECTOR(,double) & lStartingZMPPosition,
							MAL_VECTOR(,double) & lStartingWaistPose,
							FootAbsolutePosition & InitLeftFootAbsPos,
							FootAbsolutePosition & InitRightFootAbsPos)
  {
    MAL_VECTOR(lBodyInit,double);    
    MAL_VECTOR_RESIZE(lBodyInit,m_CurrentActuatedJointValues.size());
    
    for(unsigned int j=0; j<m_CurrentActuatedJointValues.size();j++)
      {
	lBodyInit(j) = m_CurrentActuatedJointValues[j];
      }

    m_GlobalStrategyManager->EvaluateStartingState(lBodyInit,
						   lStartingCOMPosition,
						   lStartingZMPPosition,
						   lStartingWaistPose,
						   InitLeftFootAbsPos, InitRightFootAbsPos);
  }

  // This method assumes that we still are using the ZMP
  // someday it should go out.
  void PatternGeneratorInterfacePrivate::AutomaticallyAddFirstStep(deque<RelativeFootPosition> & lRelativeFootPositions,
							    FootAbsolutePosition & InitLeftFootAbsPos,
							    FootAbsolutePosition & InitRightFootAbsPos,
							    COMPosition &lStartingCOMPosition)
  {
    MAL_S3x3_MATRIX(InitPos,double);
    MAL_S3x3_MATRIX(CoMPos,double);

    double coscomyaw, sincomyaw;
    coscomyaw = cos(lStartingCOMPosition.yaw);
    sincomyaw = sin(lStartingCOMPosition.yaw);
    
    CoMPos(0,0) = coscomyaw; CoMPos(0,1) = -sincomyaw; CoMPos(0,2) = lStartingCOMPosition.x[0];
    CoMPos(1,0) = sincomyaw; CoMPos(1,1) =  coscomyaw; CoMPos(1,2) = lStartingCOMPosition.y[0];
    CoMPos(2,0) = 0.0;       CoMPos(2,1) = 0.0;        CoMPos(2,2) = 1.0;

    // First step targets the left
    // then the robot should move towards the right.
    double lsx,lsy,ltheta;
    if (lRelativeFootPositions[0].sy > 0 )
      {
	lsx = InitRightFootAbsPos.x; lsy = InitRightFootAbsPos.y; ltheta = InitRightFootAbsPos.theta;
      }
    // First step targets the right
    // then the robot should move towards the left.
    else
      {
	lsx = InitLeftFootAbsPos.x; lsy = InitLeftFootAbsPos.y; ltheta = InitLeftFootAbsPos.theta;
      }

    double cosinitfoottheta, sininitfoottheta;
    cosinitfoottheta = cos(ltheta);
    sininitfoottheta = sin(ltheta);
    
    InitPos(0,0) = cosinitfoottheta; InitPos(0,1) = -sininitfoottheta; InitPos(0,2) = lsx;
    InitPos(1,0) = sininitfoottheta; InitPos(1,1) =  cosinitfoottheta; InitPos(1,2) = lsy;
    InitPos(2,0) = 0.0;              InitPos(2,1) = 0.0;               InitPos(2,2) = 1.0;
    
    ODEBUG("InitPos:" << InitPos);
    ODEBUG("CoMPos: " << CoMPos);

    MAL_S3x3_MATRIX(iCoMPos,double);
    MAL_S3x3_INVERSE(CoMPos,iCoMPos,double);
    MAL_S3x3_MATRIX(InitialMotion,double);

    // Compute the rigid motion from the CoM to the next support foot.
    MAL_S3x3_C_eq_A_by_B(InitialMotion,iCoMPos,InitPos);
      
    // Create from the rigid motion the step to be added to the list of steps.
    RelativeFootPosition aRFP;
    memset(&aRFP,0,sizeof(aRFP));
    aRFP.sx = InitialMotion(0,2); aRFP.sy = InitialMotion(1,2);
    aRFP.theta = atan2(InitialMotion(1,0),InitialMotion(0,0));

    ODEBUG("lRelativeFootPositions:"<<lRelativeFootPositions.size());
    ODEBUG("AutomaticallyAddFirstStep: "<< aRFP.sx << " " << aRFP.sy << " " <<aRFP.theta);

    lRelativeFootPositions.push_front(aRFP);
    
  }

  void PatternGeneratorInterfacePrivate::CommonInitializationOfWalking(COMPosition  & lStartingCOMPosition,
								MAL_S3_VECTOR(,double) & lStartingZMPPosition,
								MAL_VECTOR(  & BodyAnglesIni,double),
								FootAbsolutePosition & InitLeftFootAbsPos,
								FootAbsolutePosition & InitRightFootAbsPos,
							        deque<RelativeFootPosition> & lRelativeFootPositions,
								vector<double> & lCurrentJointValues,
								bool ClearStepStackHandler)
  {
    m_ZMPPositions.clear();
    m_LeftFootPositions.clear();
    m_RightFootPositions.clear();

    lCurrentJointValues.resize(m_CurrentActuatedJointValues.size());

    for(unsigned int i=0;i<m_CurrentActuatedJointValues.size();i++)
      lCurrentJointValues[i] = m_CurrentActuatedJointValues[i];

    m_DOF = m_CurrentActuatedJointValues.size();
    MAL_VECTOR_RESIZE(BodyAnglesIni,m_CurrentActuatedJointValues.size());

    for(int j=0; j<m_DOF;j++)
      {
	BodyAnglesIni(j) = lCurrentJointValues[j];
      }

    // Copy the relative foot position from the stack handler to here.
    m_StepStackHandler->CopyRelativeFootPosition(lRelativeFootPositions,ClearStepStackHandler);
    ODEBUG4("Size of lRelativeFootPositions: " << lRelativeFootPositions.size(),"DebugData.txt");
    for(unsigned int i=0;i<lRelativeFootPositions.size();i++)
      {
	ODEBUG(lRelativeFootPositions[i].sx << " " << 
		lRelativeFootPositions[i].sy << " " <<  
		lRelativeFootPositions[i].theta );

      }


    // Initialize consequently the ComAndFoot Realization object.
    MAL_VECTOR(lStartingWaistPose,double);
    m_GlobalStrategyManager->EvaluateStartingState(BodyAnglesIni,
						   lStartingCOMPosition,
						   lStartingZMPPosition,
						   lStartingWaistPose,
						   InitLeftFootAbsPos, InitRightFootAbsPos);

    // Add the first step automatically when the corresponding option is set on.
    if (m_AutoFirstStep)
      {
	AutomaticallyAddFirstStep(lRelativeFootPositions,
				  InitLeftFootAbsPos,
				  InitRightFootAbsPos,
				  lStartingCOMPosition);
	if (!ClearStepStackHandler)
	  m_StepStackHandler->PushFrontAStepInTheStack(lRelativeFootPositions[0]);
      }

    ODEBUG("StartingCOMPosition: " << lStartingCOMPosition.x[0] 
	    << " "  << lStartingCOMPosition.y[0]
	    << " "  << lStartingCOMPosition.z[0]);
    ODEBUG4("StartingCOMPosition: " << lStartingCOMPosition.x[0] << " "  << lStartingCOMPosition.y[0],"DebugData.txt");
    // We also initialize the iteration number inside DMB.
    std::cerr << "You have to implement a reset iteration number." << endl;
    //m_HumanoidDynamicRobot->ResetIterationNumber();

    ODEBUG4("CommonInitializationOfWalking " << BodyAnglesIni,"DebugData.txt" );

    if (0)
      {

	ofstream aof;
	aof.open("/tmp/output.txt", ofstream::out);
	if (aof.is_open())
	  {
	    for(unsigned int i=0;i<lRelativeFootPositions.size();i++)
	      {
		aof << lRelativeFootPositions[i].sx <<" "
		    << lRelativeFootPositions[i].sy <<" "
		    << lRelativeFootPositions[i].theta
		    << endl;
	      }
	  }
      }

  }


  void PatternGeneratorInterfacePrivate::StartOnLineStepSequencing()
  {
    COMPosition lStartingCOMPosition;
    MAL_S3_VECTOR(,double) lStartingZMPPosition;
    MAL_VECTOR( BodyAnglesIni,double);

    FootAbsolutePosition InitLeftFootAbsPos, InitRightFootAbsPos;
    memset(&InitLeftFootAbsPos,0,sizeof(InitLeftFootAbsPos));
    memset(&InitRightFootAbsPos,0,sizeof(InitRightFootAbsPos));
    
    deque<RelativeFootPosition> lRelativeFootPositions;
    vector<double> lCurrentJointValues;

    ODEBUG("StartOnLineStepSequencing - 1 ");
    m_StepStackHandler->StartOnLineStep();


    CommonInitializationOfWalking(lStartingCOMPosition,
				  lStartingZMPPosition,
				  BodyAnglesIni,
				  InitLeftFootAbsPos, InitRightFootAbsPos,
				  lRelativeFootPositions,lCurrentJointValues,false);


    if (m_ZMPInitialPointSet)
      {
	for(unsigned int i=0;i<3;i++)
	  lStartingZMPPosition(i) = m_ZMPInitialPoint(i);
      }

    ODEBUG("StartOnLineStepSequencing - 3 "
	   << lStartingCOMPosition.x[0] << " "
	   << lRelativeFootPositions.size()
	   );
    ODEBUG("ZMPInitialPoint OnLine" << lStartingZMPPosition(0)  << " "
	    << lStartingZMPPosition(1)  << " " << lStartingZMPPosition(2) );
    int NbOfStepsToRemoveFromTheStack=0;
    if (m_AlgorithmforZMPCOM==ZMPCOM_KAJITA_2003)
      {
	ODEBUG("ZMPCOM KAJITA 2003 - 2 ");
	NbOfStepsToRemoveFromTheStack=m_ZMPD->InitOnLine(m_ZMPPositions,
							 m_COMBuffer,
							 m_LeftFootPositions,
							 m_RightFootPositions,
							 InitLeftFootAbsPos,
							 InitRightFootAbsPos,
							 lRelativeFootPositions,
							 lStartingCOMPosition,
							 lStartingZMPPosition);

      }
    else if (m_AlgorithmforZMPCOM==ZMPCOM_MORISAWA_2007)
      {
	m_ZMPM->SetCurrentTime(m_InternalClock);
	NbOfStepsToRemoveFromTheStack = m_ZMPM->InitOnLine(m_ZMPPositions,
							   m_COMBuffer,
							   m_LeftFootPositions,
							   m_RightFootPositions,
							   InitLeftFootAbsPos,
							   InitRightFootAbsPos,
							   lRelativeFootPositions,
							   lStartingCOMPosition,
							   lStartingZMPPosition );
	ODEBUG("After Initializing the Analytical Morisawa part. " << m_LeftFootPositions.size()
		<< " " << m_RightFootPositions.size());
      }

    // Keep the last one to be removed at the next insertion.
    for(int i=0;i<NbOfStepsToRemoveFromTheStack-1;i++)
      m_StepStackHandler->RemoveFirstStepInTheStack();

    // Initialization of the first preview.
    for(int j=0; j<m_DOF;j++)
      {
	BodyAnglesIni(j) = lCurrentJointValues[j];
	ODEBUG4(BodyAnglesIni(j),"DebugDataOnLine.txt");
      }

    m_GlobalStrategyManager->Setup(m_ZMPPositions,
				   m_COMBuffer,
				   m_LeftFootPositions,
				   m_RightFootPositions);

    m_ShouldBeRunning=true;

    ODEBUG("StartOnLineStepSequencing - 4 "
	   << m_ZMPPositions.size() << " "
	   << m_LeftFootPositions.size() << " "
	   << m_RightFootPositions.size() << " ");
  }

  void PatternGeneratorInterfacePrivate::StopOnLineStepSequencing()
  {
    m_StepStackHandler->StopOnLineStep();
  }

  void PatternGeneratorInterfacePrivate::FinishAndRealizeStepSequence()
  { 
    ODEBUG("PGI-Start");
    COMPosition lStartingCOMPosition;	
    MAL_S3_VECTOR(,double) lStartingZMPPosition;
    MAL_VECTOR( BodyAnglesIni,double);
    FootAbsolutePosition InitLeftFootAbsPos, InitRightFootAbsPos;
    struct timeval begin, end, time4, time5;

    gettimeofday(&begin,0);

    ODEBUG("FinishAndRealizeStepSequence() - 1");

    vector<double> lCurrentJointValues;
    m_ZMPD->SetZMPShift(m_ZMPShift);

    MAL_VECTOR(,double) lCurrentConfiguration;

    lCurrentConfiguration = m_HumanoidDynamicRobot->currentConfiguration();  
    ODEBUG("lCurrent Configuration :" << lCurrentConfiguration);

    deque<RelativeFootPosition> lRelativeFootPositions;
    CommonInitializationOfWalking(lStartingCOMPosition,
				  lStartingZMPPosition,
				  BodyAnglesIni,
				  InitLeftFootAbsPos, InitRightFootAbsPos,
				  lRelativeFootPositions,lCurrentJointValues,true);
    
    ODEBUG( "Pass through here ");
    lCurrentConfiguration(0) = 0.0;
    lCurrentConfiguration(1) = 0.0;
    lCurrentConfiguration(2) = 0.0;
    lCurrentConfiguration(3) = 0.0;
    lCurrentConfiguration(4) = 0.0;
    lCurrentConfiguration(5) = 0.0;
    m_HumanoidDynamicRobot->currentConfiguration(lCurrentConfiguration);

    ODEBUG("Size of lRelativeFootPositions :" << lRelativeFootPositions.size());
    ODEBUG("ZMPInitialPoint" << lStartingZMPPosition(0)  << " "
	     << lStartingZMPPosition(1)  << " " << lStartingZMPPosition(2) );

    // Create the ZMP reference.
    CreateZMPReferences(lRelativeFootPositions,
			lStartingCOMPosition,
			lStartingZMPPosition,
			InitLeftFootAbsPos,
			InitRightFootAbsPos);

    ODEBUG("First m_ZMPPositions" << m_ZMPPositions[0].px << " " << m_ZMPPositions[0].py);
    deque<ZMPPosition> aZMPBuffer;

    // Option : Use Wieber06's algorithm to compute a new ZMP
    // profil. Suppose to preempt the first stage of control.
    aZMPBuffer.resize(m_RightFootPositions.size());

    // this function calculates a buffer with COM values after a first preview round,
    // currently required to calculate the arm swing before "onglobal step of control"
    // in order to take the arm swing motion into account in the second preview loop
    if (m_StepStackHandler->GetWalkMode()==2)
      m_StOvPl->CreateBufferFirstPreview(m_COMBuffer,aZMPBuffer,m_ZMPPositions);


    if (0)
      {
	m_ZMPD->DumpDataFiles("/tmp/ZMPSetup.dat",
			      "/tmp/LeftFootAbsolutePosSetup.dat",
			      m_ZMPPositions,
			      m_LeftFootPositions);

	m_ZMPD->DumpDataFiles("/tmp/ZMPSetup.dat",
			      "/tmp/RightFootAbsolutePosSetup.dat",
			      m_ZMPPositions,
			      m_RightFootPositions);
      }

    ODEBUG4("FinishAndRealizeStepSequence() - 5 ","DebugGMFKW.dat");
    // Link the current trajectory and GenerateMotionFromKineoWorks.

    // Very important, you have to make sure that the correct COM position is
    // set inside this buffer.
    // X and Y  will be defined by the PG, but the height has to be specified.
    // by default it should be Zc.
    // If you want to change use modewalk 2.

    ODEBUG4("FinishAndRealizeStepSequence() - 6 ","DebugGMFKW.dat");
    ODEBUG4("m_ZMPPositions : " << m_ZMPPositions.size() << endl <<
	    " m_LeftFootPositions: " << m_LeftFootPositions.size()<< endl <<
	    " m_RightFootPositions: " << m_RightFootPositions.size()<< endl <<
	    " m_TimeDistrFactor" << m_TimeDistrFactor.size() << endl <<
	    "m_COMBuffer : " << m_COMBuffer.size() << endl,"DebugGMFKW.dat");
    if(m_StepStackHandler->GetWalkMode()==2)
      {
	ODEBUG4("FinishAndRealizeStepSequence() - 6.25 ","DebugGMFKW.dat");
	m_StOvPl->TimeDistributeFactor(m_TimeDistrFactor);
	ODEBUG4("FinishAndRealizeStepSequence() - 6.5 ","DebugGMFKW.dat");
	m_StOvPl->PolyPlanner(m_COMBuffer,m_LeftFootPositions,m_RightFootPositions,m_ZMPPositions);
	ODEBUG4("FinishAndRealizeStepSequence() - 6.75 ","DebugGMFKW.dat");
      }

    gettimeofday(&time4,0);
    ODEBUG4("FinishAndRealizeStepSequence() - 7 ","DebugGMFKW.dat");
    // Read NL informations from ZMPRefPositions.
    m_GlobalStrategyManager->Setup(m_ZMPPositions,
				   m_COMBuffer,
				   m_LeftFootPositions,
				   m_RightFootPositions);

    gettimeofday(&time5,0);

    m_count = 0;
    ODEBUG("FinishAndRealizeStepSequence() - 8 ");

    m_ShouldBeRunning = true;

    m_InternalClock = 0.0;

    gettimeofday(&end,0);
  }


  void PatternGeneratorInterfacePrivate::m_ReadFileFromKineoWorks(istringstream &strm)
  {

    string aPartialModel="PartialModel.dat";
    string aKWPath="KWBarPath.pth";

    strm >> aPartialModel;
    strm >> aKWPath;

    ODEBUG6("Went through m_ReadFileFromKineoWorks(istringstream &strm)","DebugGMFKW.dat");
    if (m_GMFKW->ReadPartialModel(aPartialModel)<0)
      cerr<< "Error while reading partial model " << endl;

    if (m_GMFKW->ReadKineoWorksPath(aKWPath)<0)
      cerr<< "Error while reading the path " << endl;
    ODEBUG6("Went before DisplayModel and PAth m_ReadFileFromKineoWorks(istringstream &strm)",
	    "DebugGMFKW.dat");

    //    m_GMFKW->DisplayModelAndPath();
    ODEBUG6("Fini..","DebugGMFKW.dat");
  }

  int PatternGeneratorInterfacePrivate::ParseCmd(istringstream &strm)
  {
    string aCmd;
    strm >> aCmd;

    ODEBUG("PARSECMD");
    if (SimplePluginManager::CallMethod(aCmd,strm))
      {
	ODEBUG("Method " << aCmd << " found and handled.");
      }
    return 0;
  }
  void PatternGeneratorInterfacePrivate::ChangeOnLineStep(istringstream &strm,double &newtime)
  {
    if (m_AlgorithmforZMPCOM==ZMPCOM_MORISAWA_2007)
      {
	FootAbsolutePosition aFAP;
	double ltime = (double)m_ZMPM->GetTSingleSupport();
	strm >> aFAP.x;
	strm >> aFAP.y;
	strm >> aFAP.theta;
	ChangeOnLineStep(ltime,aFAP,newtime);
      }
  }
					     
  void PatternGeneratorInterfacePrivate::CallMethod(string &aCmd,
					     istringstream &strm)
  {

    ODEBUG("PGI:ParseCmd: Commande: " << aCmd);

    if (aCmd==":ChangeNextStep")
      {
	double nt;
	ChangeOnLineStep(strm,nt);
      }
    else if (aCmd==":LimitsFeasibility")
      m_SetLimitsFeasibility(strm);

    else if (aCmd==":ZMPShiftParameters")
      m_SetZMPShiftParameters(strm);

    else if (aCmd==":TimeDistributionParameters")
      m_SetTimeDistrParameters(strm);

    else if (aCmd==":stepseq")
      m_StepSequence(strm);

    else if (aCmd==":finish")
      m_FinishAndRealizeStepSequence(strm);

    else if (aCmd==":StartOnLineStepSequencing")
      {
	m_InternalClock = 0.0;
	ReadSequenceOfSteps(strm);
	StartOnLineStepSequencing();
      }
    else if (aCmd==":StopOnLineStepSequencing")
      StopOnLineStepSequencing();

    else if (aCmd==":readfilefromkw")
      m_ReadFileFromKineoWorks(strm);

    else if (aCmd==":SetAlgoForZmpTrajectory")
      m_SetAlgoForZMPTraj(strm);

    else if (aCmd==":SetAutoFirstStep")
      {
	std::string lAutoFirstStep;
	strm>> lAutoFirstStep;
	if (lAutoFirstStep=="true")
	  m_AutoFirstStep=true;
	else  if (lAutoFirstStep=="false")
	  m_AutoFirstStep=false;
	ODEBUG("SetAutoFirstStep: " << m_AutoFirstStep);
      }

  }

  void PatternGeneratorInterfacePrivate::m_SetAlgoForZMPTraj(istringstream &strm)
  {
    string ZMPTrajAlgo;
    strm >> ZMPTrajAlgo;
    ODEBUG("ZMPTrajAlgo: " << ZMPTrajAlgo);
    if (ZMPTrajAlgo=="PBW")
      {
	m_AlgorithmforZMPCOM = ZMPCOM_WIEBER_2006;
	m_GlobalStrategyManager = m_DoubleStagePCStrategy;
      }
    else if (ZMPTrajAlgo=="Kajita")
      {
	m_AlgorithmforZMPCOM = ZMPCOM_KAJITA_2003;
	m_GlobalStrategyManager = m_DoubleStagePCStrategy;
      }
    else if (ZMPTrajAlgo=="Morisawa")
      {
	m_AlgorithmforZMPCOM = ZMPCOM_MORISAWA_2007;
	m_GlobalStrategyManager = m_CoMAndFootOnlyStrategy;
	m_CoMAndFootOnlyStrategy->SetTheLimitOfTheBuffer(m_ZMPM->ReturnOptimalTimeToRegenerateAStep());
      }
    else if (ZMPTrajAlgo=="Dimitrov")
      {
	m_AlgorithmforZMPCOM = ZMPCOM_DIMITROV_2008;
	m_GlobalStrategyManager = m_DoubleStagePCStrategy;
	cout << "DIMITROV" << endl;
      }
  }

  void PatternGeneratorInterfacePrivate::m_SetUpperBodyMotionParameters(istringstream &strm)
  {
    ODEBUG("Upper Body Motion Parameters");
    while(!strm.eof())
      {
      }
  }



  bool PatternGeneratorInterfacePrivate::RunOneStepOfTheControlLoop(MAL_VECTOR(,double) & CurrentConfiguration,
							     MAL_VECTOR(,double) & CurrentVelocity,
							     MAL_VECTOR(,double) & CurrentAcceleration,
							     MAL_VECTOR( &ZMPTarget,double))
  {
    COMPosition finalCOMPosition;
    FootAbsolutePosition LeftFootPosition,RightFootPosition;

    return RunOneStepOfTheControlLoop(CurrentConfiguration,
				      CurrentVelocity,
				      CurrentAcceleration,
				      ZMPTarget,
				      finalCOMPosition,
				      LeftFootPosition,
				      RightFootPosition);
    
  }

  bool PatternGeneratorInterfacePrivate::RunOneStepOfTheControlLoop(FootAbsolutePosition &LeftFootPosition,
							     FootAbsolutePosition &RightFootPosition,
							     ZMPPosition &ZMPRefPos,
							     COMPosition &COMRefPos)
  {
    MAL_VECTOR(,double)  CurrentConfiguration;
    MAL_VECTOR(,double)  CurrentVelocity;
    MAL_VECTOR(,double)  CurrentAcceleration;
    MAL_VECTOR( ZMPTarget,double);
    bool r=false;

    r = RunOneStepOfTheControlLoop(CurrentConfiguration,
				   CurrentVelocity,
				   CurrentAcceleration,
				   ZMPTarget,
				   COMRefPos,
				   LeftFootPosition,
				   RightFootPosition);

    bzero(&ZMPRefPos,sizeof(ZMPPosition));
    ZMPRefPos.px = ZMPTarget(0);
    ZMPRefPos.py = ZMPTarget(1);
    return r;
  }


  bool PatternGeneratorInterfacePrivate::RunOneStepOfTheControlLoop(MAL_VECTOR(,double) & CurrentConfiguration,
							     MAL_VECTOR(,double) & CurrentVelocity,
							     MAL_VECTOR(,double) & CurrentAcceleration,
							     MAL_VECTOR( &ZMPTarget,double),
							     COMPosition &finalCOMPosition,
							     FootAbsolutePosition &LeftFootPosition, 
							     FootAbsolutePosition &RightFootPosition )


  {

    m_InternalClock+=m_SamplingPeriod;

    if ((!m_ShouldBeRunning) ||
	(m_GlobalStrategyManager->EndOfMotion()<0))
      {
	ODEBUG(" m_ShoulBeRunning " << m_ShouldBeRunning << endl <<
		" m_ZMPPositions " << m_ZMPPositions.size() << endl <<
		" 2*m_NL+1 " << 2*m_NL+1 << endl);
	ODEBUG("m_ShouldBeRunning : "<< m_ShouldBeRunning << endl <<
	       "m_GlobalStrategyManager: " << m_GlobalStrategyManager->EndOfMotion());
		
	return false;
      }
    ODEBUG("Here");

    if (m_StepStackHandler->IsOnLineSteppingOn())
      {
	ODEBUG("On Line Stepping: ON!");
	// ********* WARNING THIS IS THE TIME CONSUMING PART *******************
	if (m_AlgorithmforZMPCOM==ZMPCOM_WIEBER_2006)
	  {
	  }
	else if (m_AlgorithmforZMPCOM==ZMPCOM_KAJITA_2003)
	  {
	    m_ZMPD->OnLine(m_InternalClock,
			   m_ZMPPositions,
			   m_COMBuffer,
			   m_LeftFootPositions,			   
			   m_RightFootPositions);
	  }
	else if (m_AlgorithmforZMPCOM==ZMPCOM_MORISAWA_2007)
	  {
	    m_ZMPM->OnLine(m_InternalClock,
			   m_ZMPPositions,
			   m_COMBuffer,
			   m_LeftFootPositions,
			   m_RightFootPositions);
	  }
      }
    else 
      /* Check if we are not in an ending phase generated on-line */
      {
	if ((m_AlgorithmforZMPCOM==ZMPCOM_MORISAWA_2007) &&
	    (m_ZMPM->GetOnLineMode()))
	  {
	    m_ZMPM->OnLine(m_InternalClock,
			   m_ZMPPositions,
			   m_COMBuffer,
			   m_LeftFootPositions,
			   m_RightFootPositions);
	  }
      }
    

    m_GlobalStrategyManager->OneGlobalStepOfControl(LeftFootPosition,
						    RightFootPosition,
						    ZMPTarget,
						    finalCOMPosition,
						    CurrentConfiguration,
						    CurrentVelocity,
						    CurrentAcceleration);

    
    // New scheme:
    // Update the queue of ZMP ref
    m_count++;
    
    // Update the waist state, it is assumed that the waist is the free flyer 
    // Depending on the strategy used to generate the CoM trajectory
    // this can be empty.

    m_CurrentWaistState.x[0]  = CurrentConfiguration[0];
    m_CurrentWaistState.y[0]  = CurrentConfiguration[1];
    m_CurrentWaistState.z[0]  = CurrentConfiguration[2];
    m_CurrentWaistState.roll  = CurrentConfiguration[3];
    m_CurrentWaistState.pitch = CurrentConfiguration[4];
    m_CurrentWaistState.yaw   = CurrentConfiguration[5];

    m_CurrentWaistState.x[1]  = CurrentVelocity[0];
    m_CurrentWaistState.y[1]  = CurrentVelocity[1];
    m_CurrentWaistState.z[1]  = CurrentVelocity[2];

    ODEBUG4("CurrentWaistState: " 
	    << m_CurrentWaistState.x[0] << " " 
	    << m_CurrentWaistState.y[0] << " " 
	    << m_CurrentWaistState.z[0] << " "
	    << m_CurrentWaistState.roll << " "
	    << m_CurrentWaistState.pitch << " "
	    << m_CurrentWaistState.yaw,
	    "DebugDataWaist.dat" );

    bool UpdateAbsMotionOrNot = false;

    //    if ((u=(m_count - (m_ZMPPositions.size()-2*m_NL)))>=0)
    if (m_GlobalStrategyManager->EndOfMotion()==GlobalStrategyManager::NEW_STEP_NEEDED)
      {
	ODEBUG("NEW STEP NEEDED" << m_InternalClock/m_SamplingPeriod << " Internal Clock :" << m_InternalClock);
	if (m_StepStackHandler->IsOnLineSteppingOn())
	  {
	    // CAREFULL: we assume that this sequence will create a
	    // a new foot steps at the back of the queue handled by the StepStackHandler.
	    // Then we have two foot steps: the last one put inside the preview,
	    // and the new one.
	    RelativeFootPosition lRelativeFootPositions;
	    // Add a new step inside the stack.
	    if (m_StepStackHandler->ReturnStackSize()<=1)
	      {
		m_StepStackHandler->AddStandardOnLineStep(m_NewStep, m_NewStepX, m_NewStepY, m_NewTheta);
		m_NewStep = false;
	      }

	    // Remove the first step of the queue.
	    bool EndSequence = m_StepStackHandler->RemoveFirstStepInTheStack();

	    // Returns the front foot step in the step stack handler which is not yet
	    // in the preview control queue.
	    bool EnoughSteps= m_StepStackHandler->ReturnFrontFootPosition(lRelativeFootPositions);
	    if ((!EnoughSteps)&& (!EndSequence))
	      {
		ODEBUG3("You don't have enough steps in the step stack handler.");
		ODEBUG3("And this is not an end sequence.");
	      }


	    ODEBUG("Asking a new step");
	    
	    if (!EndSequence)
	      {
		// ********* WARNING THIS IS THE TIME CONSUMING PART *******************
		if (m_AlgorithmforZMPCOM==ZMPCOM_WIEBER_2006)
		  {
		  }
		else if (m_AlgorithmforZMPCOM==ZMPCOM_KAJITA_2003)
		  {
		    m_ZMPD->OnLineAddFoot(lRelativeFootPositions,
					  m_ZMPPositions,
					  m_COMBuffer,
					  m_LeftFootPositions,			   
					  m_RightFootPositions,
					  EndSequence);
		  }
		else if (m_AlgorithmforZMPCOM==ZMPCOM_MORISAWA_2007)
		  {
		    ODEBUG("Putting a new step SX: " << 
			   lRelativeFootPositions.sx << " SY: " 
			   << lRelativeFootPositions.sy );
		    m_ZMPM->SetCurrentTime(m_InternalClock);
		    m_ZMPM->OnLineAddFoot(lRelativeFootPositions,
					  m_ZMPPositions,
					  m_COMBuffer,
					  m_LeftFootPositions,
					  m_RightFootPositions,
					  EndSequence);
		    ODEBUG("Left and Right foot positions queues: " 
			   << m_LeftFootPositions.size() << " " 
			   << m_RightFootPositions.size() );
		  }
	      }
	    else if (EndSequence)
	      {
		if (m_AlgorithmforZMPCOM==ZMPCOM_WIEBER_2006)
		  {
		  }
		else if (m_AlgorithmforZMPCOM==ZMPCOM_KAJITA_2003)
		  {
		    m_ZMPD->EndPhaseOfTheWalking(m_ZMPPositions,
						 m_COMBuffer,
						 m_LeftFootPositions,			   
						 m_RightFootPositions);
		  }
		else if (m_AlgorithmforZMPCOM==ZMPCOM_MORISAWA_2007)
		  {
		    ODEBUG("Putting a new step SX: " << 
			   lRelativeFootPositions.sx << " SY: " 
			   << lRelativeFootPositions.sy );
		    m_ZMPM->SetCurrentTime(m_InternalClock);
		    m_ZMPM->EndPhaseOfTheWalking(m_ZMPPositions,
						 m_COMBuffer,
						 m_LeftFootPositions,
						 m_RightFootPositions);
		    ODEBUG("("<<m_InternalClock << ")");
		    ODEBUG("Left and Right foot positions queues: " 
			   << m_LeftFootPositions.size() << " " 
			   << m_RightFootPositions.size() );
		  }
	      }
	    // ************* THIS HAS TO FIT INSIDE THE control step time  ***********

	  }
	else
	  {

	    //	cout << "Sorry not enough information" << endl;
	    m_ShouldBeRunning = false;
	    UpdateAbsMotionOrNot = true;

	    if ((m_AlgorithmforZMPCOM==ZMPCOM_MORISAWA_2007) &&
		(m_ZMPM->GetOnLineMode()))
	      {
		m_ShouldBeRunning = true;	    
	      }

	    ODEBUG("Finished the walking pattern generator ("<<m_InternalClock << ")");
	  }

	/*
	ODEBUG3("CurrentActuatedJointValues at the end: " );
	for(unsigned int i=0;i<m_CurrentActuatedJointValues.size();i++)
	  cout << m_CurrentActuatedJointValues[i] << " " ;
	cout << endl;
	*/
	ODEBUG4("*** TAG *** " , "DebugDataIK.dat");

      }

    // Update the absolute position of the robot.
    // to be done only when the robot has finish a motion.
    UpdateAbsolutePosition(UpdateAbsMotionOrNot);
    return true;
  }



  void PatternGeneratorInterfacePrivate::m_SetTimeDistrParameters(istringstream &strm)
  {
    ODEBUG("SetTimeDistrParameters");
    while(!strm.eof())
      {
	if (!strm.eof())
	  {
	    strm >> m_TimeDistrFactor[0];
	  }
	else break;
	if (!strm.eof())
	  {
	    strm >> m_TimeDistrFactor[1];

	  }
	else break;
	if (!strm.eof())
	  {
	    strm >> m_TimeDistrFactor[2];

	  }
	else break;
	if (!strm.eof())
	  {
	    strm >> m_TimeDistrFactor[3];

	  }
	else break;
      }
  }

  void PatternGeneratorInterfacePrivate::SetCurrentJointValues(MAL_VECTOR( & lCurrentJointValues,double))
  {
    if(MAL_VECTOR_SIZE(lCurrentJointValues)!=m_CurrentActuatedJointValues.size())
      m_CurrentActuatedJointValues.resize(MAL_VECTOR_SIZE(lCurrentJointValues));

    for(unsigned int i=0;i<MAL_VECTOR_SIZE(lCurrentJointValues);i++)
      {
	m_CurrentActuatedJointValues[i] = lCurrentJointValues(i);
      }
  }


  void PatternGeneratorInterfacePrivate::m_FinishAndRealizeStepSequence(istringstream &strm)
  {
    int Synchronize=1;

    while(!strm.eof())
      {
	if (!strm.eof())
	  strm >> Synchronize;
	else
	  break;
      }
    FinishAndRealizeStepSequence();
  }


  int PatternGeneratorInterfacePrivate::GetWalkMode()
  {
    return m_StepStackHandler->GetWalkMode();
  }


  void PatternGeneratorInterfacePrivate::m_PartialStepSequence(istringstream &strm)
  {
    if (m_StepStackHandler!=0)
      m_StepStackHandler->m_PartialStepSequence(strm);
  }

  void PatternGeneratorInterfacePrivate::GetLegJointVelocity(MAL_VECTOR( & dqr,double),
						      MAL_VECTOR( & dql,double))
  {

    // TO DO: take the joint specific to the legs
    // and create the appropriate vector.
    for(int i=0;i<6;i++)
      {
	dqr(i) = m_dqr(i);
	dql(i) = m_dql(i);
      }
  }

  void PatternGeneratorInterfacePrivate::ExpandCOMPositionsQueues(int aNumber)
  {
    COMPosition aCOMPos;
    KWNode anUpperBodyPos;

    for(int i=0;i<aNumber;i++)
      {
	// Add COM value set at a default value.
	aCOMPos.z[0] = m_PC->GetHeightOfCoM();
	aCOMPos.z[1] = 0.0;
	aCOMPos.z[2] = 0.0;

	aCOMPos.pitch = 0.0;
	aCOMPos.roll = 0.0;
	m_COMBuffer.push_back(aCOMPos);

	// Add UpperBody Position set at a default value.
      }

  }



  void PatternGeneratorInterfacePrivate::AddOnLineStep(double X, double Y, double Theta)
  {
    m_NewStep = true;
    m_NewStepX = X;
    m_NewStepY = Y;
    m_NewTheta = Theta;
  }

  void PatternGeneratorInterfacePrivate::UpdateAbsolutePosition(bool UpdateAbsMotionOrNot)
  {
    // Compute relative, absolution position and speed.
    MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistRelativePos, 3,0) = 0.0;
    MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistRelativePos, 3,1) = 0.0;
    MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistRelativePos, 3,2) = 0.0;
    MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistRelativePos, 3,3) = 1.0;

    double thetarad = m_CurrentWaistState.yaw;
    double c = cos(thetarad);
    double s = sin(thetarad);

    MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistRelativePos, 0,0) = c;
    MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistRelativePos, 0,1)=-s;
    MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistRelativePos, 0,2) = 0;

    MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistRelativePos, 1,0) = s;
    MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistRelativePos, 1,1)= c;
    MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistRelativePos, 1,2) = 0;

    MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistRelativePos, 2,0) = 0;
    MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistRelativePos, 2,1)= 0;
    MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistRelativePos, 2,2) = 1;

    MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistRelativePos, 3,3) = 1;

    MAL_S4_VECTOR( RelativeLinearVelocity,double);
    RelativeLinearVelocity(0) =  m_CurrentWaistState.x[1];
    RelativeLinearVelocity(1) =  m_CurrentWaistState.y[1];
    RelativeLinearVelocity(2) =  m_CurrentWaistState.z[0];
    RelativeLinearVelocity(3) =  1.0;

    MAL_S4_VECTOR( RelativeLinearAcc,double);
    RelativeLinearAcc(0) =  m_CurrentWaistState.x[2];
    RelativeLinearAcc(1) =  m_CurrentWaistState.y[2];
    RelativeLinearAcc(2) =  0.0;
    RelativeLinearAcc(3) =  1.0;

    MAL_S4x4_C_eq_A_by_B(m_AbsLinearVelocity,
			 m_MotionAbsOrientation,
			 RelativeLinearVelocity);
    MAL_S4x4_C_eq_A_by_B(m_AbsLinearAcc,
			 m_MotionAbsOrientation ,
			 RelativeLinearAcc);

    MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistRelativePos, 0,3) = m_CurrentWaistState.x[0];
    MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistRelativePos, 1,3) = m_CurrentWaistState.y[0];
    MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistRelativePos, 2,3) = m_CurrentWaistState.z[0];

    MAL_S4x4_MATRIX( prevWaistAbsPos,double);
    prevWaistAbsPos = m_WaistAbsPos;

    MAL_S4x4_C_eq_A_by_B(m_WaistAbsPos, m_MotionAbsPos , m_WaistRelativePos);

    ODEBUG("Motion Abs Pos " << m_MotionAbsPos);
    ODEBUG("Waist Relative Pos " << m_WaistRelativePos);
    ODEBUG("Waist Abs Pos " << m_WaistAbsPos);

    m_AbsAngularVelocity(0) = 0.0;
    m_AbsAngularVelocity(1) = 0.0;

    if (m_count!=0)
      m_AbsAngularVelocity(2) = (m_AbsMotionTheta + thetarad - m_AbsTheta )/m_dt;
    else
      m_AbsAngularVelocity(2) = 0.0;

    m_AbsAngularVelocity(3) = 1.0;
    //      cout << "m_AbsAngularVelocity " << m_AbsAngularVelocity<< endl;
    m_AbsTheta = fmod(m_AbsMotionTheta + thetarad,2*M_PI);

    if (UpdateAbsMotionOrNot)
      {
	m_MotionAbsPos = m_WaistAbsPos;
	// The position is supposed at the ground level
	MAL_S4x4_MATRIX_ACCESS_I_J(m_MotionAbsPos, 2,3) = 0.0;
	m_AbsMotionTheta = m_AbsTheta;
      }

  }

  void PatternGeneratorInterfacePrivate::getWaistPositionMatrix(MAL_S4x4_MATRIX( &lWaistAbsPos,double))
  {
    lWaistAbsPos = m_WaistAbsPos;
  }

  void PatternGeneratorInterfacePrivate::getWaistPositionAndOrientation(double aTQ[7], double &Orientation)
  {
    // Position
    aTQ[0] = MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistAbsPos, 0,3);
    aTQ[1] = MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistAbsPos, 1,3);
    aTQ[2] = MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistAbsPos, 2,3);


    // Carefull : Extremly specific to the pattern generator.
    double cx,cy,cz, sx,sy,sz;
    cx = 0; cy = 0; cz = cos(0.5*m_AbsTheta);
    sx = 0; sy = 0; sz = sin(0.5*m_AbsTheta);
    aTQ[3] = 0;
    aTQ[4] = 0;
    aTQ[5] = sz;
    aTQ[6] = cz;
    Orientation = m_AbsTheta;
  }

  void PatternGeneratorInterfacePrivate::setWaistPositionAndOrientation(double aTQ[7])
  {
    // Position
    MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistAbsPos, 0,3) = aTQ[0];
    MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistAbsPos, 1,3) = aTQ[1];
    MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistAbsPos, 2,3) = aTQ[2];
    double _x = aTQ[3];
    double _y = aTQ[4];
    double _z = aTQ[5];
    double _r = aTQ[6];


    double x2 = _x * _x;
    double y2 = _y * _y;
    double z2 = _z * _z;
    double r2 = _r * _r;
    // fill diagonal terms
    MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistAbsPos, 0,0) = r2 + x2 - y2 - z2;
    MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistAbsPos, 1,1) = r2 - x2 + y2 - z2;
    MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistAbsPos, 2,2) = r2 - x2 - y2 + z2;
    double xy = _x * _y;
    double yz = _y * _z;
    double zx = _z * _x;
    double rx = _r * _x;
    double ry = _r * _y;
    double rz = _r * _z;
    // fill off diagonal terms
    MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistAbsPos, 0,1) = 2 * (xy - rz);
    MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistAbsPos, 0,2) = 2 * (zx + ry);
    MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistAbsPos, 1,0) = 2 * (xy + rz);
    MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistAbsPos, 1,2) = 2 * (yz - rx);
    MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistAbsPos, 2,0) = 2 * (zx - ry);
    MAL_S4x4_MATRIX_ACCESS_I_J(m_WaistAbsPos, 2,1) = 2 * (yz + rx);

  }

  void PatternGeneratorInterfacePrivate::getWaistVelocity(double & dx,
						   double & dy,
						   double & omega)
  {
    dx = m_AbsLinearVelocity(0);
    dy = m_AbsLinearVelocity(1);
    omega = m_AbsAngularVelocity(2);
  }


  int PatternGeneratorInterfacePrivate::ChangeOnLineStep(double time,
						  FootAbsolutePosition & aFootAbsolutePosition,
						  double &newtime)
  {
    /* Compute the index of the interval which will be modified. */
    if (m_AlgorithmforZMPCOM==ZMPCOM_MORISAWA_2007)
      {
	m_ZMPM->SetCurrentTime(m_InternalClock);
	m_ZMPM->OnLineFootChange(m_InternalClock+time,
				 aFootAbsolutePosition,
				 m_ZMPPositions,
				 m_COMBuffer,
				 m_LeftFootPositions,
				 m_RightFootPositions,
				 m_StepStackHandler);
	vector<double> lDj;
	m_FeetTrajectoryGenerator->GetDeltaTj(lDj);
	newtime = lDj[0];
	return 0;
      }
    return -1;
  }

  int PatternGeneratorInterfacePrivate::CreateZMPReferences(deque<RelativeFootPosition> &lRelativeFootPositions,
						     COMPosition &lStartingCOMPosition,
						     MAL_S3_VECTOR(&,double) lStartingZMPPosition,
						     FootAbsolutePosition &InitLeftFootAbsPos, 
						     FootAbsolutePosition &InitRightFootAbsPos)
  {
    if (m_AlgorithmforZMPCOM==ZMPCOM_WIEBER_2006)
      {
	ODEBUG3("ZMPCOM_WIEBER_2006 " << m_ZMPPositions.size() );
	m_COMBuffer.clear();
	m_ZMPQP->GetZMPDiscretization(m_ZMPPositions,
				      m_COMBuffer,
				      lRelativeFootPositions,
				      m_LeftFootPositions,
				      m_RightFootPositions,
				      m_Xmax, lStartingCOMPosition,
				      lStartingZMPPosition,
				      InitLeftFootAbsPos,
				      InitRightFootAbsPos);      
      }    
    else if (m_AlgorithmforZMPCOM==ZMPCOM_DIMITROV_2008)
      {
	ODEBUG3("ZMPCOM_DIMITROV_2008 " << m_ZMPPositions.size() );
	m_COMBuffer.clear();
	m_ZMPCQPFF->GetZMPDiscretization(m_ZMPPositions,
					 m_COMBuffer,
					 lRelativeFootPositions,
					 m_LeftFootPositions,
					 m_RightFootPositions,
					 m_Xmax, lStartingCOMPosition,
					 lStartingZMPPosition,
					 InitLeftFootAbsPos,
					 InitRightFootAbsPos);      
      }    
    else if (m_AlgorithmforZMPCOM==ZMPCOM_KAJITA_2003)
      {
	ODEBUG3("ZMPCOM_KAJITA_2003 " << m_ZMPPositions.size() );
	m_ZMPD->GetZMPDiscretization(m_ZMPPositions,
				     m_COMBuffer,
				     lRelativeFootPositions,
				     m_LeftFootPositions,
				     m_RightFootPositions,
				     m_Xmax, lStartingCOMPosition,
				     lStartingZMPPosition,
				     InitLeftFootAbsPos,
				     InitRightFootAbsPos);
	//	m_COMBuffer.clear();
	//	m_COMBuffer.resize(m_RightFootPositions.size());
      }
    else if (m_AlgorithmforZMPCOM==ZMPCOM_MORISAWA_2007)
      {
	ODEBUG3("ZMPCOM_MORISAWA_2007");
	m_ZMPM->GetZMPDiscretization(m_ZMPPositions,
				     m_COMBuffer,
				     lRelativeFootPositions,
				     m_LeftFootPositions,
				     m_RightFootPositions,
				     m_Xmax, lStartingCOMPosition,
				     lStartingZMPPosition,
				     InitLeftFootAbsPos,
				     InitRightFootAbsPos);

	ODEBUG3("ZMPCOM_MORISAWA_2007 " << m_ZMPPositions.size() );
      }
    return 0;
  }
  
  void PatternGeneratorInterfacePrivate::AddStepInStack(double dx, double dy, double theta)
  {
    if (m_StepStackHandler!=0)
      {
	m_StepStackHandler->AddStepInTheStack(dx,dy,theta,m_TSsupport, m_TDsupport);
      }
  }
  
  void PatternGeneratorInterfacePrivate::setZMPInitialPoint(MAL_S3_VECTOR(&,double) lZMPInitialPoint)
  {
    m_ZMPInitialPoint = lZMPInitialPoint;
    m_ZMPInitialPointSet = true;
  }

  void PatternGeneratorInterfacePrivate::getZMPInitialPoint(MAL_S3_VECTOR(&,double) lZMPInitialPoint)
  {
    lZMPInitialPoint = m_ZMPInitialPoint;
  }


  PatternGeneratorInterface * patternGeneratorInterfaceFactory(CjrlHumanoidDynamicRobot *aHDR)
  {
    return new PatternGeneratorInterfacePrivate(aHDR);
  }


  
}

