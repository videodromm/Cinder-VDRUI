uniform vec3 iResolution;uniform sampler2D iChannel0;uniform float iZoom;uniform float iRenderXYX;uniform float iRenderXYY;
uniform float iTime;uniform float iTempoTime;uniform float iRatio;uniform float iVignette;uniform float iToggle;
uniform float iExposure;uniform float iSobel;uniform float iChromatic;uniform float iGreyScale;uniform vec3 iColor;
uniform float iFlipV;uniform float iFlipH;uniform float iInvert;uniform float iTrixels;uniform float iFactor;uniform float iAlpha;
uniform float iPixelate;uniform float iGlitch;
uniform float       iRedMultiplier;			// red multiplier 
uniform float       iGreenMultiplier;		// green multiplier 
uniform float       iBlueMultiplier;		// blue multiplier 
uniform vec4 iMouse; // mouse pixel coords. xy: current (if MLB down), zw: click
const float PI = 3.1415926535897932384626433832795;
const float PI180 = float(PI / 180.0);
vec2  fragCoord = gl_FragCoord.xy;
// halftone https://www.shadertoy.com/view/ltfcRM
float sind(float a)
{
    return sin(a * PI180);
}

float cosd(float a)
{
    return cos(a * PI180);
}

float added(vec2 sh, float sa, float ca, vec2 c, float d)
{
    return 0.5 + 0.25 * cos((sh.x * sa + sh.y * ca + c.x) * d) + 0.25 * cos((sh.x * ca - sh.y * sa + c.y) * d);
}
vec4 halftone(  ) { 
    // Halftone dot matrix shader
    // @author Tomek Augustyn 2010
    
    // Ported from my old PixelBender experiment
    // https://github.com/og2t/HiSlope/blob/master/src/hislope/pbk/fx/halftone/Halftone.pbk
    
    // Hold and drag horizontally to adjust the threshold

    float threshold = 0.75;

    float ratio = iResolution.y / iResolution.x;
    float coordX = fragCoord.x / iResolution.x;
    float coordY = fragCoord.y / iResolution.x;
    vec2 dstCoord = vec2(coordX, coordY);
    vec2 srcCoord = vec2(coordX, coordY / ratio);
    vec2 rotationCenter = vec2(0.5, 0.5);
    vec2 shift = dstCoord - rotationCenter;

    float dotSize = 6.0;//clamp(float(iMouse.x / iResolution.x) + 2.6, 3.0, 10.0);
    float angle = 30.0;
    
    vec3 rasterPattern = vec3(
        added(shift, sind(angle + 00.0), cosd(angle), rotationCenter, PI / dotSize * 680.0),
        added(shift, sind(angle + 30.0), cosd(angle), rotationCenter, PI / dotSize * 680.0),
        added(shift, sind(angle + 60.0), cosd(angle), rotationCenter, PI / dotSize * 680.0)
    );
    
    vec4 srcPixel = texture(iChannel0, srcCoord);
       
    return vec4(
        (rasterPattern.r * threshold + srcPixel.r - threshold) / (1.0 - threshold),
        (rasterPattern.g * threshold + srcPixel.g - threshold) / (1.0 - threshold),
        (rasterPattern.b * threshold + srcPixel.b - threshold) / (1.0 - threshold),
        1.0
    );

}
// halftone end
float intensity(in vec4 c){return sqrt((c.x*c.x)+(c.y*c.y)+(c.z*c.z));}
vec4 sobel(float stepx, float stepy, vec2 center) {
	float tleft = intensity(texture(iChannel0,center + vec2(-stepx,stepy))); float left = intensity(texture(iChannel0,center + vec2(-stepx,0)));
	float bleft = intensity(texture(iChannel0,center + vec2(-stepx,-stepy))); float top = intensity(texture(iChannel0,center + vec2(0,stepy)));
	float bottom = intensity(texture(iChannel0,center + vec2(0,-stepy)));float tright = intensity(texture(iChannel0,center + vec2(stepx,stepy)));
	float right = intensity(texture(iChannel0,center + vec2(stepx,0)));float bright = intensity(texture(iChannel0,center + vec2(stepx,-stepy)));
	float x = tleft + 2.0*left + bleft - tright - 2.0*right - bright;float y = -tleft - 2.0*top - tright + bleft + 2.0 * bottom + bright;
	return vec4(sqrt((x*x) + (y*y)));
}
vec4 chromatic( vec2 uv ) {	
	vec2 offset = vec2(iChromatic / 36., .0);
	return vec4(texture(iChannel0, uv + offset.xy).r,  texture(iChannel0, uv).g, texture(iChannel0, uv + offset.yx).b, 1.0);
}
vec4 squares( vec2 inUV, sampler2D tex ) {
	vec4 rtn = texture(tex, inUV);
	// random squares controlled by iFactor
    vec2 gridUV = (inUV - 0.5) * 2.0;
    float gridSize = 6.0 / iFactor; // iFactor controls square density 20
    vec2 gridPos = floor(gridUV * gridSize);
    float random = fract(sin(dot(gridPos, vec2(12.9898, 78.233)) + iTime*0.000005) * 43758.5453);
    
    // threshold for square appearance
    float threshold = 0.2 + (iFactor - 1.0) * 0.3;//0.5
    if (random > threshold) {
        rtn = vec4(iColor.rgb + texture(tex, inUV).rgb, iAlpha); // squares
    }
    
    
	return rtn;
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
// glitch begin
//2D (returns 0 - 1)
float random2d(vec2 n) { 
    return fract(sin(dot(n, vec2(12.9898, 4.1414))) * 43758.5453);
}

float randomRange (in vec2 seed, in float min, in float max) {
		return min + random2d(seed) * (max - min);
}

// return 1 if v inside 1d range
float insideRange(float v, float bottom, float top) {
   return step(bottom, v) - step(top, v);
}
vec4 glitch( vec2 inUV )
{
	// glitch https://www.shadertoy.com/view/MtXBDs
	vec4 rtn;
	float time = floor(iTime * 6.5);
	//randomly offset slices horizontally
    float maxOffset = iGlitch/iRatio;
    for (float i = 0.0; i < 10.0 * iGlitch; i += 1.0) {
        float sliceY = random2d(vec2(time , 2345.0 + float(i)));
        float sliceH = random2d(vec2(time , 9035.0 + float(i))) * 0.25;
        float hOffset = randomRange(vec2(time , 9625.0 + float(i)), -maxOffset, maxOffset);
        vec2 uvOff = inUV;
        uvOff.x += hOffset;
        if (insideRange(inUV.y, sliceY, fract(sliceY+sliceH)) == 1.0 ){
        	rtn = texture(iChannel0, uvOff).rgba;
        }
    }
	//do slight offset on one entire channel
    float maxColOffset = iGlitch/iRatio;
    float rnd = random2d(vec2(time , 9545.0));
    vec2 colOffset = vec2(randomRange(vec2(time , 9545.0),-maxColOffset,maxColOffset), 
                       randomRange(vec2(time , 7205.0),-maxColOffset,maxColOffset));
    if (rnd < 0.33){
        rtn.r = texture(iChannel0, inUV + colOffset).r;
        
    }else if (rnd < 0.66){
        rtn.g = texture(iChannel0, inUV + colOffset).g;
        
    } else{
        rtn.b = texture(iChannel0, inUV + colOffset).b;  
    }
	return rtn;
}
// glitch end
vec4 greyScale( vec4 colored )
{
   return vec4( (colored.r+colored.g+colored.b)/3.0 );
}
void main() {
	vec2 uv = gl_FragCoord.xy / iResolution.xy;
	// zoom centered
	if ( iZoom != 1.0 )
	{
	  float xZ = (uv.x - 0.5)*(1.0-iZoom)*2.0;
	  float yZ = (uv.y - 0.5)*(1.0-iZoom)*2.0;
	  vec2 cZ = vec2(xZ, yZ);
	  uv = uv+cZ;
	}
	// x axis
	if ( iRenderXYX != 0.0 )
	{
	  uv.x -= iRenderXYX;
	}
	// y axis
	if ( iRenderXYY != 0.0 )
	{
	  uv.y -= iRenderXYY;
	}
	// flip horizontally
	/*if (iFlipH > 0.0)
	{
		uv.x = 1.0 - uv.x;
	}*/
	// flip vertically
	if (iFlipV == 0.0)
	{
		uv.y = 1.0 - uv.y;
	}
	if ( iPixelate < 1.0 )
	{
		vec2 divs = vec2(iResolution.x * iPixelate / iResolution.y*60.0, iPixelate*60.0);
		uv = floor(uv * divs)/ divs;
	}
	vec4 t0 = texture(iChannel0, uv);
	vec4 c = vec4(0.0);
	if (iSobel > 0.03) { t0 = sobel(iSobel * 3.0 /iResolution.x, iSobel * 3.0 /iResolution.y, uv); }
	if (iChromatic > 0.0) { t0 = chromatic(uv) * t0; }
	if (iTrixels > 0.0) { t0 = trixels( uv, iChannel0 ); }
	if (iFactor > 0.0) { t0 = squares( uv, iChannel0 ); }
	if (iGlitch > 0.0) { t0 = glitch( uv ); }
	if (iToggle > 0.0) t0 = halftone();
	c = t0;c *= iExposure;
	if (iInvert > 0.0) { c.r = 1.0 - c.r; c.g = 1.0 - c.g; c.b = 1.0 - c.b; }
	//if (iToggle > 0.0) { c.rgb = c.brg; }
	if (iGreyScale > 0.0) { c = greyScale( c ); }
	c.r *= iRedMultiplier;
	c.g *= iGreenMultiplier;
	c.b *= iBlueMultiplier;
	// old vignette
	/* if (iVignette > 0.0) { 
		vec2 p = 1.0 + -2.0 * uv;
		p.y *= 0.4;
		c = mix( c, vec4( 0.1 ), dot( p, p )*iVignette*3.0 ); 
	}
	// new vignette cyrano circle
	if (iVignette > 0.0) { 
		vec2 p = 1.0 + -2.0 * uv;
		p.y *= 1.1;
		p.x *= 2.0;
		c -= pow(length(p), 500.0);
	}*/
	// https://www.shadertoy.com/view/4lSXDm
	if (iVignette > 0.0) {
		vec2 p = 1.0 + -2.0 * uv;
		float rf = sqrt(dot(p, p)) * iVignette;
		float rf2_1 = rf * rf + 1.0;
		float e = 1.0 / (rf2_1 * rf2_1);
		c *= e;
	}
   	gl_FragColor = c;
}
