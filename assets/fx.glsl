uniform vec3 iResolution;uniform sampler2D iChannel0; uniform sampler2D iChannel1;
uniform float iTime;uniform float iFreq0;uniform float iGreyScale; uniform float iGlitch;
uniform vec4 iMouse; // mouse pixel coords. xy: current (if MLB down), zw: click
uniform float iExposure;uniform float iSobel;uniform float iChromatic;
uniform float iFlipV;uniform float iFlipH;uniform float iInvert;uniform float iTrixels;
uniform float iPixelate;
uniform float       iRedMultiplier;			// red multiplier 
uniform float       iGreenMultiplier;		// green multiplier 
uniform float       iBlueMultiplier;		// blue multiplier 
// TODO 20240520 https://www.shadertoy.com/view/lXSSRz

vec2  fragCoord = gl_FragCoord.xy;
// https://www.shadertoy.com/view/Xss3zr


const int VideoHeightFieldSteps = 64;
const vec3 VideoHeightFieldLightDir = vec3(0.577, 0.577, 0.577);

// transforms
vec3 VideoHeightFieldRotateX(vec3 p, float a)
{
    float sa = sin(a);
    float ca = cos(a);
    vec3 r;
    r.x = p.x;
    r.y = ca*p.y - sa*p.z;
    r.z = sa*p.y + ca*p.z;
    return r;
}

vec3 VideoHeightFieldRotateY(vec3 p, float a)
{
    float sa = sin(a);
    float ca = cos(a);
    vec3 r;
    r.x = ca*p.x + sa*p.z;
    r.y = p.y;
    r.z = -sa*p.x + ca*p.z;
    return r;
}

bool VideoHeightFieldIntersectBox(vec3 ro, vec3 rd, vec3 boxmin, vec3 boxmax, out float tnear, out float tfar)
{
	// compute intersection of ray with all six bbox planes
	vec3 invR = 1.0 / rd;
	vec3 tbot = invR * (boxmin - ro);
	vec3 ttop = invR * (boxmax - ro);
	// re-order intersections to find smallest and largest on each axis
	vec3 tmin = min (ttop, tbot);
	vec3 tmax = max (ttop, tbot);
	// find the largest tmin and the smallest tmax
	vec2 t0 = max (tmin.xx, tmin.yz);
	tnear = max (t0.x, t0.y);
	t0 = min (tmax.xx, tmax.yz);
	tfar = min (t0.x, t0.y);
	// check for hit
	bool hit;
	if ((tnear > tfar)) 
		hit = false;
	else
		hit = true;
	return hit;
}

float VideoHeightFieldLuminance(sampler2D tex, vec2 uv)
{
	vec3 c = texture2D(tex, uv).xyz;
	return dot(c, vec3(0.33, 0.33, 0.33));
}

vec2 gradient(sampler2D tex, vec2 uv, vec2 texelSize)
{
	float h = VideoHeightFieldLuminance(tex, uv);
	float hx = VideoHeightFieldLuminance(tex, uv + texelSize*vec2(1.0, 0.0));	
	float hy = VideoHeightFieldLuminance(tex, uv + texelSize*vec2(0.0, 1.0));
	return vec2(hx - h, hy - h);
}

vec2 VideoHeightFieldWorldToTex(vec3 p)
{
	vec2 uv = p.xz*0.5+0.5;
	uv.y = 1.0 - uv.y;
	return uv;
}

float VideoHeightFieldHeightField(vec3 p)
{
	return VideoHeightFieldLuminance(iChannel0, VideoHeightFieldWorldToTex(p))*iFreq0/150.0;
}

bool VideoHeightFieldTraceHeightField(vec3 ro, vec3 rayStep, out vec3 hitPos)
{
	vec3 p = ro;
	bool hit = false;
	float pH = 0.0;
	vec3 pP = p;
	for(int i=0; i<VideoHeightFieldSteps; i++) {
		float h = VideoHeightFieldHeightField(p);
		if ((p.y < h) && !hit) {
			hit = true;
			//hitPos = p;
			// interpolate based on height
            hitPos = mix(pP, p, (pH - pP.y) / ((p.y - pP.y) - (h - pH)));
		}
		pH = h;
		pP = p;
		p += rayStep;
	}
	return hit;
}
float intensity(in vec4 c){return sqrt((c.x*c.x)+(c.y*c.y)+(c.z*c.z));}
vec4 sobel(float stepx, float stepy, vec2 center) {
	float tleft = intensity(texture(iChannel0,center + vec2(-stepx,stepy))); float left = intensity(texture(iChannel0,center + vec2(-stepx,0)));
	float bleft = intensity(texture(iChannel0,center + vec2(-stepx,-stepy))); float top = intensity(texture(iChannel0,center + vec2(0,stepy)));
	float bottom = intensity(texture(iChannel0,center + vec2(0,-stepy)));float tright = intensity(texture(iChannel0,center + vec2(stepx,stepy)));
	float right = intensity(texture(iChannel0,center + vec2(stepx,0)));float bright = intensity(texture(iChannel0,center + vec2(stepx,-stepy)));
	float x = tleft + 2.0*left + bleft - tright - 2.0*right - bright;float y = -tleft - 2.0*top - tright + bleft + 2.0 * bottom + bright;
	return vec4(sqrt((x*x) + (y*y)));
}
vec3 chromatic(in vec3 c, vec2 uv ) {	
	vec2 offset = vec2(iChromatic / 36., .0);
	// return vec3(texture(iChannel0, uv + offset.xy).r,  texture(iChannel0, uv).g, texture(iChannel0, uv + offset.yx).b);
	return vec3(c.r + (uv.x + offset.x)*iResolution.x - 0.5, c.g, c.b + offset.y);
}
vec4 trixels( vec2 inUV, sampler2D tex )
{
	// trixels https://www.shadertoy.com/view/4lj3Dm
	vec4 rtn;
	
    float height = iResolution.x/(1.01 - iTrixels)/90.0;
    float halfHeight = height*0.5;
    float halfBase = height/sqrt(3.0);
    float base = halfBase*2.0;

    float screenX = gl_FragCoord.x;
    float screenY = gl_FragCoord.y;    

    float upSlope = height/halfBase;
    float downSlope = -height/halfBase;

    float oddRow = mod(floor(screenY/height),2.0);
    screenX -= halfBase*oddRow;
    
    float oddColumn = mod(floor(screenX/halfBase), 2.0);

    float localX = mod(screenX, halfBase);
    float localY = mod(screenY, height);

    if(oddColumn == 0.0 )
    {
        if(localY >= localX*upSlope)
        {
            screenX -= halfBase;
        }
    }
    else
    {
        if(localY <= height+localX*downSlope)
        {
            screenX -= halfBase;
        }
    }
    
    float startX = floor(screenX/halfBase)*halfBase;
    float startY = floor(screenY/height)*height;
    vec4 blend = vec4(0.0);
    for(float x = 0.0; x < 3.0; x += 1.0)
    {
        for(float y = 0.0; y < 3.0; y += 1.0)
        {
            vec2 screenPos = vec2(startX+x*halfBase,startY+y*halfHeight);
            vec2 uv1 = screenPos / iResolution.xy;
			uv1.y = 1.0 - uv1.y;
			blend += texture2D(tex, uv1);         
        }
    }
    rtn = (blend / 9.0);
   	return rtn;
}
// trixels end
vec3 greyScale( vec3 colored )
{
   return vec3( (colored.r+colored.g+colored.b)/3.0 );
}
// start glitch
// Ruofei Du
// Dot Screen / Halftone: https://www.shadertoy.com/view/4sBBDK
// Halftone: https://www.shadertoy.com/view/lsSfWV
/*mat2 rotate2d(float angle) {
	return mat2(cos(angle), -sin(angle), sin(angle), cos(angle));
}
float dotScreen(in vec2 uv, in float angle, in float scale) {
	float s = sin(angle), c = cos(angle);
	vec2 p = (uv - vec2(0.5)) * iResolution.xy;
	vec2 q = rotate2d(angle) * p * scale;
	return (sin(q.x) * sin(q.y)) * 4.0;
}
vec3 glitch(vec3 col, vec2 pos)
{
	float angle = iMouse.w < 1e-3 ? 0.4 : iMouse.z / iMouse.w;
	float scale = 1.0 + 0.3 * sin(iTime);
	return col * 10.0 - 5.0 + dotScreen(pos, angle, scale);
}*/


// Created by starea - 2016
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0
#define C_RED vec3(1.0, 0.0, 0.0)
#define C_YELLOW vec3(1.0, 1.0, 0.0)
#define C_BLUE vec3(0.5, 0.0, 1.0)
// start https://www.shadertoy.com/view/4llcDl
float val = 4.0;

vec3 toyuv(vec3 color) {
	return vec3(color.r * 0.299 + color.g * 0.587 + color.b * 0.114,
		color.r * -0.169 + color.g * -0.331 + color.b * 0.5,
		color.r * 0.5 + color.g * -0.419 + color.b * -0.081);
}

vec3 torgb(vec3 i) {
	vec3 yuva = i;//vec3(i.x, (i.y - 0.5), (i.z - 0.5));
	return vec3(yuva.x * 1.0 + yuva.y * 0.0 + yuva.z * 1.4,
		yuva.x * 1.0 + yuva.y * -0.343 + yuva.z * -0.711,
		yuva.x * 1.0 + yuva.y * 1.765 + yuva.z * 0.0);
}
// end https://www.shadertoy.com/view/4llcDl
vec3 glitch(vec3 c, vec2 pos)
{
	float luminance = 0.299 * c.r + 0.587 * c.g + 0.114 * c.b;
	float THRESHOLD = 0.5;// (length(iMouse.zw) < 1e-2) ? 0.5 : iMouse.z / iResolution.x;
	vec3 rtn = (luminance < THRESHOLD) ? mix(C_BLUE, C_YELLOW, luminance * 2.0) : mix(C_YELLOW, C_RED, (luminance - 0.5) * 2.0);
	if (iMouse.z >0.1) {
		// https://www.shadertoy.com/view/wdSBz3
		const float frequency = 510.;
		const float scale = 1.2;
		float sine = sin(frequency*pos.x) * sin(frequency*pos.y);
		rtn *= sine * sine > scale*luminance ?
			vec3(199., 0., 57.) / 255. :
			vec3(252., 195., 5.) / 255.;
	}
	if (iMouse.w > 0.1) {
		// https://www.shadertoy.com/view/4llcDl
		vec3 yuvC = toyuv(c);
		vec3 newCol = vec3(yuvC.x,
			yuvC.y * sin(iTime) + yuvC.z * cos(iTime),
			yuvC.y * cos(iTime) - yuvC.z * sin(iTime));

		vec3 newColor = torgb(newCol);
		rtn *= newColor;
	}
	return rtn.rgb *= 0.1 + 0.25 + 0.75 * pow(16.0 * pos.x * pos.y * (1.0 - pos.x) * (1.0 - pos.y), 0.15);
}
// end glitch

void main() {
 	vec2 pos = gl_FragCoord.xy / iResolution.xy;

 	vec2 uv = 0.5 * vec2( (gl_FragCoord.x / iResolution.x)*1.2-0.6, (gl_FragCoord.y / iResolution.y)*-2.4+1.2 );
    uv.x *= float(iResolution.x )/ float(iResolution.y);
	uv.y *= 1.5;

	// compute ray origin and direction
    float asp = iResolution.x / iResolution.y;
    vec3 rd = normalize(vec3(asp*uv.x, uv.y, -2.0));
    vec3 ro = vec3(0.0, 0.0, 2.0);

	// rotate view	
    float ax = -1.0 + (iMouse.x - 0.4) * 6.42;	
    rd = VideoHeightFieldRotateX(rd, ax);
    ro = VideoHeightFieldRotateX(ro, ax);
		
	float ay = 0.0 + (iMouse.y - 0.565) * 2.5;
    rd = VideoHeightFieldRotateY(rd, ay);
    ro = VideoHeightFieldRotateY(ro, ay);
	
	// intersect with bounding box
    bool hit;	
	const vec3 boxMin = vec3(-1.0, -0.01, -1.0);
	const vec3 boxMax = vec3(1.0, 0.5, 1.0);
	float tnear, tfar;
	hit = VideoHeightFieldIntersectBox(ro, rd, boxMin, boxMax, tnear, tfar);

	tnear -= 0.0001;
	vec3 pnear = ro + rd*tnear;
    vec3 pfar = ro + rd*tfar;
	
    float stepSize = length(pfar - pnear) / float(VideoHeightFieldSteps);
	vec3 c = vec3(0.0);
    if(hit)
    {
    	// intersect with heightfield
		ro = pnear;
		vec3 hitPos;
		hit = VideoHeightFieldTraceHeightField(ro, rd*stepSize, hitPos);
		if (hit) {
			vec2 uvt = VideoHeightFieldWorldToTex(hitPos);
			c = texture2D(iChannel0, uvt).xyz;
		}
    }
	//vec4 t0 = c.rgbr;//texture(iChannel0, uv);
	//if (iSobel > 0.03) { c.rgb = sobel(iSobel * 3.0 /iResolution.x, iSobel * 3.0 /iResolution.y, uv); }
	if (iChromatic > 0.0) { c = (chromatic(c, uv) * c); }
	//if (iTrixels > 0.0) { t0 = trixels( uv, iChannel0 ); }

	if (iInvert > 0.0) { c.r = 1.0 - c.r; c.g = 1.0 - c.g; c.b = 1.0 - c.b; }

	if (iGreyScale > 0.0) { c = greyScale( c ); }
	if (iGlitch > 0.0) { c = glitch(c, pos); }

   	gl_FragColor = vec4(c, 1.0);
}
