#ifndef TEST
//new SM3 single-pass FS
char *fragmentShaderAssembly = 
"varying vec2 position;\n"
"uniform vec2 center;\n"
"uniform float scale;\n"
"uniform vec3 innerColor;\n"
"uniform sampler1D palette;\n"
"uniform float maxIterations;\n"
"void main (){\n"
	"vec2 c = vec2(position.x - center.x, position.y - center.y);\n"
	"vec2 z = c;\n"
	"gl_FragColor = vec4(innerColor, 1.0);\n"
	"for(float i = 0.0; i < maxIterations; i += 1.0){\n"
		"z = vec2(z.x*z.x - z.y*z.y, 2.0*z.x*z.y) + c;\n"
		"if (dot(z, z) > 4.0){\n"
			"gl_FragColor = texture1D(palette, i / maxIterations);\n"
			"break;\n"
		"}\n"
	"}\n"
"}\n\0";
#else
//***test
char *fragmentShaderAssembly = 
"void main (){\n"
	"gl_FragColor = vec4(gl_TexCoord[0].xy, 0.0,1.0);\n"
"}\n\0";
#endif

#ifndef TEST
//SM2 single pass shader - maxIterations is int, otherwise not used in loop on Ati DX10 HW
char *fragmentShaderAssembly2 = 
"varying vec2 position;\n"
"uniform vec2 center;\n"
"uniform float scale;\n"
"uniform vec3 innerColor;\n"
"uniform sampler1D palette;\n"
"uniform int maxIterations;\n"
"void main (){\n"
	"vec2 c = vec2(position.x - center.x, position.y - center.y);\n"
	"vec2 z = c;\n"
	"float done = 0.0;\n"
	"gl_FragColor = vec4(innerColor, 1.0);\n"
	"for(int i = 0; i < maxIterations; ++i){\n"
		"z = vec2(z.x*z.x - z.y*z.y, 2.0*z.x*z.y) + c;\n"
		"if (dot(z, z) > 4.0 && done < 0.1){\n"
			"done=float(i);\n"
		"}\n"
	"}\n"
	"gl_FragColor = texture1D(palette, done / float(maxIterations));\n"
"}\n\0";
#else
//***test
char *fragmentShaderAssembly2 = 
"uniform vec3 innerColor;\n"
"uniform sampler1D palette;\n"
"uniform int maxIterations;\n"
"void main (){\n"
	"float done = 0.0;\n"
	"gl_FragColor = vec4(innerColor, 1.0);\n"
	"for(int i = 0; i < maxIterations; ++i){\n"
		//"done = i/(maxIterations + 1);\n"
		"if (done < 1.0){\n"
			"done = 127.0;\n"
		"}\n"
	"}\n"
	//"gl_FragColor = texture1D(palette, done/maxIterations);\n"
	"gl_FragColor = vec4((done+127.0)/float(maxIterations)*gl_TexCoord[0].xy,  0.0, 1.0);\n"
	//"gl_FragColor = vec4(maxIterations/maxIterations*gl_TexCoord[0].xy,  0.0, 1.0);\n"
"}\n\0";
#endif


#ifndef TEST
char *fragShaderMPInit = 
"varying vec2 position;"
"uniform vec2 center;\n"
"void main(){\n"
	"vec2 c = vec2(position.x - center.x, position.y - center.y);\n"
	"gl_FragColor = vec4(c, 0.0, 0.0);\n"
"}\n\0";
#else
//***test
char *fragShaderMPInit = 
"void main(){\n"
	"vec2 c = gl_TexCoord[0].xy;\n"
	"gl_FragColor = vec4(c, 0.0, 0.0);\n"
"}\n\0";
#endif

#ifndef TEST
char *fragShaderMP =
"varying vec2 position;\n"
"uniform vec2 center;\n"
"uniform float scale;\n"
"uniform sampler2D inputTex;\n"
"uniform float curLoop;\n"
"void main(){\n"
	"vec4 inputValue = texture2D(inputTex, gl_TexCoord[0].xy);\n"
	"vec2 z = inputValue.xy;\n"
	"vec2 c = vec2(position.x - center.x, position.y - center.y);\n"
	"vec4 outputValue = vec4(0.0,0.0,0.0,1.0);\n"
	"for(float i=0.0; i < 5.0; i+=1.0){\n"	
		"if (dot(z, z) > 4.0){\n"
			"outputValue.xy = z;\n"
			"outputValue.z = max(inputValue.z, outputValue.z);\n"
		"}else{\n"			
			"z = vec2(z.x*z.x - z.y*z.y, 2.0*z.x*z.y) + c;\n"
			"outputValue.xy = z;\n"
			"outputValue.z = curLoop * 5.0 + i;\n"
		"}\n"
	"}\n"
	"gl_FragColor = outputValue;\n"
"}\n\0";

#else
//***test
char *fragShaderMP =
"uniform sampler2D inputTex;\n"
"void main(){\n"
	"gl_FragColor = texture2D(inputTex, gl_TexCoord[0].xy);\n"
"}\n\0";
#endif

#ifndef TEST
char *fragShaderMPFinish =
"varying vec2 position;"
"uniform vec3 innerColor;\n"
"uniform sampler1D palette;\n"
"uniform sampler2D inputTex;\n"
"uniform float maxIterations;\n"
"void main(){\n"
	"vec4 inputValue = texture2D(inputTex, gl_TexCoord[0].xy);\n"
	"vec2 z = inputValue.xy;\n"
	"if (dot(z, z) < 4.0)\n"
		"gl_FragColor = vec4(innerColor, 1.0);\n"
	"else\n"
		"gl_FragColor = texture1D(palette, (inputValue.z) / maxIterations);\n"
"}\n\0";

#else
//***test
char *fragShaderMPFinish =
"uniform sampler2D inputTex;\n"
"void main(){\n"
	"gl_FragColor = texture2D(inputTex, gl_TexCoord[0].xy);\n"
"}\n\0";
#endif

#ifndef TEST
//new single-pass VS
//invert position on Y-axix for OpenGL texture coordinates
char *vertexShaderAssembly = 
"varying vec2 position;\n"
"uniform float scale;\n"
"void main(){\n"
	"gl_Position = ftransform();\n"
	"position = vec2(1.3333*(gl_MultiTexCoord0.x - 0.5), -gl_MultiTexCoord0.y + 0.5) * scale;\n"
	"gl_TexCoord[0] = gl_MultiTexCoord0;\n"
"}\n\0";
#else
//***test VS
char *vertexShaderAssembly = 
"void main(){\n"
	"gl_Position = ftransform();\n"
	//"gl_FrontColor = gl_Color;\n"
	"gl_TexCoord[0] = gl_MultiTexCoord0;\n"
"}\n\0";
#endif