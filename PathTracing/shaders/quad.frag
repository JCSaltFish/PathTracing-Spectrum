#version 410

uniform sampler2D tex;
uniform int channel = 0;

in vec2 tex_coord;
out vec4 fragcolor;

void main()
{
	vec4 res = texelFetch(tex, ivec2(gl_FragCoord), 0);
	fragcolor = texelFetch(tex, ivec2(gl_FragCoord), 0);
	if (channel == 0)
		fragcolor = res;
	else if (channel == 1)
		fragcolor = vec4(res.rrr, 1.0);
	else if (channel == 2)
		fragcolor = vec4(res.ggg, 1.0);
	else if (channel == 3)
		fragcolor = vec4(res.bbb, 1.0);
}
