/** 
\mainpage 
\section sec_intro Introduction

This library implements the Walking Pattern Generator as described by Kajita
in his IROS 2003 paper \ref Kajita2003 . It is build upon the preview control of the 3D inverted pendulum.
Compare to the commercial version shipped with the HRP-2,
new functionnalities were added. Those functionnalities are dynamical stepping-over
and upper-body motion.


\section walkGenJrl_sec_approach Approach implemented

The immediate available reference for this code is \ref Kajita2003 .
But numerous details were given in Kajita's book \ref Kajita2005 .

\section walkGenJrl_sec_organization Organization of the code

The code relative to the Walking Pattern Generator library is divided into three parts:
\li the library itself,
\li a plugin which can be used inside the OpenHRP architecture and accessed
through CORBA,
\li several examples with a console interface,
\li a Qt GUI.

\image html PatternGenerator/Stacks.png "Architecture of the Pattern Generator's stacks."
\image latex PatternGenerator/Stacks.pdf "Architecture of the Pattern Generator's stacks."

\subsection subsec_WalkGenJRL_library The library libWalkGenJRL.so

Library libWalkGenJRL.so is aimed at implementing a walking pattern generator for humanoid robots 
having 6 DOFs per legs, and for which a VRML file is available.

\subsubsection subsubsec_WalkGenJRL_plugin Plugin 

The plugin allows to use the walking pattern generator inside the OpenHRP architecture.
As it is also a CORBA server, it can be used also by other components of the robotic application.

\section sec_changes Changes between v1.0 and v2.0
The v2.0 library has the following new functionnalities:
\li It takes properly into account the upper body position inside the pattern generator.
\li Based on experimental data, the height of the ZMP can be changed also the Ricatti
system is not modified.
\li Bjorn Verrelst's stepping over is now fully integrated.
\li It is possible to specify way-points for the upper-body while walking.


\section References
\anchor Kajita2003,
Shuuji Kajita and Fumio Kanehiro and Kenji Kaneko and Kiyoshi Fujiwara and Kensuke Harada and Kazuhito Yokoi and Hirohisa Hirukawa, "Biped Walking Pattern Generation by using Preview Control of Zero-Moment Point",
International Conference on Robotics And Automation, 2003, Taipei Taiwan 

\anchor Kajita2005,
Shuuji Kajita, Omsha, Humanoid Robot ,
2005,(In Japanese) ISBN4-274-20058-2

\anchor Verrelst2006,
Bjorn Verrelst and Kazuhito Yokoi and Olivier Stasse and Hitoshi Arisumi and Bram Vanderborght,
"Mobility of Humanoid Robots: Stepping over Large Obstacles Dynamically",
International Conference on Mechatronics and Automation

\anchor Morisawa2007
M. Morisawa and K. Harada and S. Kajita and S. Nakaoka and K. Fujiwara and F. Kanehiro and K. Kaneko and H. Hirukawa,
"Experimentation of Humanoid Walking Allowing Immediate Modification of Foot Place Based on Analytical Solution",
IEEE Int. Conf. on Robotics and Automation,
3989--3994, 2007

\anchor Wieber2006
P.-B. Wieber
Trajectory Free Linear Model Predictive Control for Stable Walking in the Presence of Strong Perturbations
IEEE/RAS Int. Conf. on Humanoid Robots
137-142, 2006



   \defgroup pgjrl JRL Walking Pattern Generator Library (WalkGenJRL)
   @{

   This library is intended to implement walking pattern generator algorithms for humanoid robots.

*/

/**
   \defgroup walkGenJrl_steppingover Stepping Over

   This group implements the stepping over algorithm as presented
   by Bjorn Verrelst \ref Verrelst2006 .
*/

/**
   \defgroup walkGenJrl_previewcontrol Preview Control

   This group implements the preview control algorithm for the cart-model
   as presented by Kajita in \ref Kajita2003 .

*/

/**
   \defgroup walkGenJrl_geometry Geometry
   
   This group implements some basic geometrical tools for the Pattern Generator.
   @{
*/


/**
   @}
*/

/**
   \defgroup walkGenJrl_Interface Interface for the PatternGeneratorJRL

   This group reinforces the independance between the internal
   structure of the Walking Pattern Generator and external
   algorithms. The main class is \a PatternGeneratorInterface
   which allow to handle very simply the WPG.
*/

/**
   @}
*/

/**
   \defgroup pginterfaces  Interfaces to WalkGenJRL
   This group shows how to interface the WalkGenJRL library to three kinds of applications:
   \li An OpenHRP plugin, to run in real-time inside an HRP-2 humanoid robot,
   \li A console program, for quick but yet complete testing of the pattern generator,
   \li An OpenGL interface.

*/