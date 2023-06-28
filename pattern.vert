#version 330 compatibility

uniform float		xLight;
uniform float		yLight;
uniform float		zLight;

out	vec2	vST;	// texture coords

out	vec3	vN;		// normal vector
out	vec3	vL;		// vector from point to light
out	vec3	vE;		// vector from point to eye

vec3 LightPosition = vec3(xLight, yLight, zLight);

void
main( )
{
	vST = gl_MultiTexCoord0.st;
	vec3 vert = gl_Vertex.xyz;

	// Per-fragment lighing
	vec4 ECposition = gl_ModelViewMatrix * vec4( vert, 1. );
	vN = normalize( gl_NormalMatrix * gl_Normal );	// normal vector
	vL = LightPosition - ECposition.xyz;			// vector from the point
													// to the light position
	vE = vec3( 0., 0., 0. ) - ECposition.xyz;		// vector from the point
													// to the eye position

	gl_Position = gl_ModelViewProjectionMatrix * vec4( vert, 1. );
}
