# OpenGL Gear Transmission
![image](https://github.com/EvgenyOvechnikov/OpenGLGearTransmission/assets/61941266/9f40e3b7-bfc6-4ba2-b70f-b5e885d534da)<br/>

Please check out the full demo video with explanations here: (markup syntax doesn't allow to embed it)<br/>
https://media.oregonstate.edu/media/t/1_jz6vcoas<br/>
<br/>
The solution should run out of the box in Visual studio. When run, use mouse to move the scene. Use the keys to do the following:<br/>
x - Toggle axis system on/off,<br/>
f - Freeze animation,<br/>
l - Toggle contact control lines,<br/>
c - Toggle corrosion on/off<br/>
<br/>
In the mechanical engineering the transmission gear teeth cannot be of arbitrary height and angular width, because of the strength requirements. Parameters of cylindrical gear teeth are calculated from a so-called “Module”, which is taken from a row of standard values. However, for the scope of computer graphics project we can afford to arbitrarily assign these parameters. Instead of setting up gear ratio, I decided to set up numbers of teeth for gear 1 and gear 2. Gear ratio can easily be calculated as number of teeth 2 / number of teeth 1. Same thing applies to the gear radius.<br/>
<br/>
The following parameters were set for our transmission:
Number of teeth on the gear 1:	23<br/>
Number of teeth on the gear 2:	47<br/>
Radius of gear 1: 10.<br/>
Height of teeth: 2.<br/>
Thickness in Z direction:	1.<br/>
Number of arms on the gear 1:	3<br/>
Number of arms on the gear 2:	5<br/>
Generalized number of polygons:	20<br/>
<br/>
The resulting scene:<br/>
![image](https://github.com/EvgenyOvechnikov/OpenGLGearTransmission/assets/61941266/7f0a0ef6-f7f2-41d4-b497-4c0f15632eff)<br/>
<br/>
The first challenge was to draw involute for radius 10 and end it precisely at radius 12 (radius 1 + teeth height). We need to split the curve to 20 polygons, with the last point ending up at the distance of 12. The involute equation which involves x and y coordinates is this:<br/>
x=r(cos ⁡t+t*sin ⁡t);<br/>
y=r(sin⁡ t-t*cos ⁡t),<br/>
where r is Radius 1 and t is involute angle.<br/>
<br/>
The condition such that the point is on the outer radius of teeth is this:<br/>
x^2 + y^2 = 12^2.<br/>
I use simple binary search to solve the equations numerically. If the required angle is between 0 and π/2, binary search finds the solution for less than 30 iterations with the precision of 0.0000001.<br/>
<br/>
When x, y and t are found, it’s easy to distinguish the inner 20 points and the angular size of that involute around the gear axis, which is going to be used in angular teeth widths. I couldn’t come up with the precise solution for perfect contact between two gears, so I introduced coefficient that defines ratio between upper tooth angular with and its lower angular width, and it takes a couple of iterations to adjust that coefficient manually. This is the resulting contact picture:<br/>
![image](https://github.com/EvgenyOvechnikov/OpenGLGearTransmission/assets/61941266/155323bb-c66d-421b-8a01-68802c15d8c3)<br/>
We see very good precision and contacts happen precisely on the contact lines.<br/>
Animation is pretty straightforward, the second gear rotates Num of teeth 2 / Num of teeth 1 times slower than gear 1.<br/>
<br/>
The next challenging thing is lighting. I assigned a blue metallic color to gear 1, and some rusty color to gear 2. To achieve metal shininess I use vertex and fragment shaders to apply per fragment lightning. Also, I use s texture coordinate as a variable to transfer a random value between 0. and 9., which is used in the fragment shader to decide if the fragment should be discarded.<br/>
![image](https://github.com/EvgenyOvechnikov/OpenGLGearTransmission/assets/61941266/747fe4c8-6aca-4a7d-abc8-5fc30b89b92a)<br/>
<br/>
Thank you. Have fun!
