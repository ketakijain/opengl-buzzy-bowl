# opengl-buzzy-bowl
3D UAV simulation show for football game day.
This is a part of the final project assignment for ECE 6122-Advanced Programming Techniques course.
It involves creating a 3D simulation of a UAV show using OpenGL with C++.

Description:  
The show is made up of 15 UAVs that are placed on the football field at the 0, 25, 50, 25, 
0 yard-lines. The UAVs remain on the ground for 5 seconds after the beginning of the simulation.  
After the initial 5 seconds the UAVs then launch from the ground and go towards a
point above the ground.As they approach the point,  they began to fly in random paths along the 
surface of a virtual sphere  while attempting to maintain a speed between 
2 to 10 m/s.The simulation ends once all of the UAV have come within 10 m of the point, (0, 0, 50 
m), and the UAVs have flown along the surface for 60 seconds.The UAVs have been given the texture of the obj "Suzanne".
This is implemented in the form of a multithreaded application. Each UAV has a thread of its own that describes the motion.  

MOTION:  
The initial motion towards the centre point is defined by kinematic equations for each UAV, the final position being the same.  
The logic for motion along the surface of the sphere is as follows:  
First, each UAV chooses a random point generated on the surface of a sphere, using a normal distribution to choose theta and phi. The point is then described parametrically using equation of sphere:  
x= r*sinϕcosθ  
y=r*sinϕsinθ  
z=r*cosϕ  
where r is the radius of the sphere




