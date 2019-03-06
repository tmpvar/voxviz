float cosine_hash(float seed)
{
    return fract(sin(seed)*43758.5453 );
}

vec3 cosine_direction( in float seed, in vec3 nor)
{
    float u = cosine_hash( 78.233 + seed);
    float v = cosine_hash( 10.873 + seed);


    // Method 1 and 2 first generate a frame of reference to use with an arbitrary
    // distribution, cosine in this case. Method 3 (invented by fizzer) specializes
    // the whole math to the cosine distribution and simplfies the result to a more
    // compact version that does not depend on a full frame of reference.

    #if 0
        // method 1 by http://orbit.dtu.dk/fedora/objects/orbit:113874/datastreams/file_75b66578-222e-4c7d-abdf-f7e255100209/content
        vec3 tc = vec3( 1.0+nor.z-nor.xy*nor.xy, -nor.x*nor.y)/(1.0+nor.z);
        vec3 uu = vec3( tc.x, tc.z, -nor.x );
        vec3 vv = vec3( tc.z, tc.y, -nor.y );

        float a = 6.2831853 * v;
        return sqrt(u)*(cos(a)*uu + sin(a)*vv) + sqrt(1.0-u)*nor;
    #endif
	#if 0
    	// method 2 by pixar:  http://jcgt.org/published/0006/01/01/paper.pdf
    	float ks = (nor.z>=0.0)?1.0:-1.0;     //do not use sign(nor.z), it can produce 0.0
        float ka = 1.0 / (1.0 + abs(nor.z));
        float kb = -ks * nor.x * nor.y * ka;
        vec3 uu = vec3(1.0 - nor.x * nor.x * ka, ks*kb, -ks*nor.x);
        vec3 vv = vec3(kb, ks - nor.y * nor.y * ka * ks, -nor.y);

        float a = 6.2831853 * v;
        return sqrt(u)*(cos(a)*uu + sin(a)*vv) + sqrt(1.0-u)*nor;
    #endif
    #if 1
    	// method 3 by fizzer: http://www.amietia.com/lambertnotangent.html
        float a = 6.2831853 * v;
        u = 2.0*u - 1.0;
        return normalize( nor + vec3(sqrt(1.0-u*u) * vec2(cos(a), sin(a)), u) );
    #endif
}
