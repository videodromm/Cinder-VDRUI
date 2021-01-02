uniform vec3 iResolution;uniform sampler2D iChannel0;uniform float iZoom;
uniform float iExposure;uniform float iSobel;uniform float iChromatic;
uniform float iFlipV;uniform float iFlipH;uniform float iInvert;uniform float iTrixels;
vec2  fragCoord = gl_FragCoord.xy;
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
    vec4 blend = vec4(0.0,0.0,0.0,0.0);
    for(float x = 0.0; x < 3.0; x += 1.0)
    {
        for(float y = 0.0; y < 3.0; y += 1.0)
        {
            vec2 screenPos = vec2(startX+x*halfBase,startY+y*halfHeight);
            vec2 uv1 = screenPos / iResolution.xy;
			blend += texture2D(tex, uv1);         
        }
    }
    rtn = (blend / 9.0);
   	return rtn;
}
// trixels end
void main() {
	vec2 uv = gl_FragCoord.xy / iResolution.xy;
	if (iFlipH > 0.0)
	{
		uv.x = 1.0 - uv.x;
	}
	// flip vertically
	if (iFlipV > 0.0)
	{
		uv.y = 1.0 - uv.y;
	}

	vec4 t0 = texture(iChannel0, uv );vec4 c = vec4(0.0);
	if (iSobel > 0.03) { t0 = sobel(iSobel * 3.0 /iResolution.x, iSobel * 3.0 /iResolution.y, uv); }
	if (iChromatic > 0.0) { t0 = chromatic(uv) * t0; }
	if (iTrixels > 0.0) { t0 = trixels( uv, iChannel0 ); }
	c = t0;c *= iExposure;
	if (iInvert > 0.0) { c = 1.0 - c; }
   	gl_FragColor = c;
}
