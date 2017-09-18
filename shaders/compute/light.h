const float pi = 3.1415926535;

float OrenNayer(vec3 normal,vec3 light,vec3 view,float a,float r2){
    // Simple Lambert
    float LV = max(dot(light,view),0.0);
    float LN = max(dot(light,normal),0.0);
    float NV = max(dot(normal,view),0.0);
    float l = (a/pi)*LN;
    // Full
    // Compute A
    float A = 1.0 + r2 * (a / (r2 + 0.13) + 0.5 / (r2 + 0.57));//1.0 - 0.5 / (r2 + 0.57);
    // Compute B
    float B = r2+0.09;
    B = (0.45*r2)/B;
    // Compute light
    float s = LV-LN*NV;
    float t = mix(1.0,max(LN,NV),step(0.0,s));
    l = l*(A+B*s/t);

    return l;
}

vec3 Fresnel(float VdotH, vec3 F0){
    return F0 + (1.0 - F0) * pow(1.0 - clamp(VdotH,0.0,1.0), 5.0);
}

float PGeom(float cosThN,float alpha){
    float cosTh2 = clamp(cosThN*cosThN,0.0,1.0);
    float tan2   = (1.0-cosTh2)/cosTh2;
    return 2.0/(1.0+sqrt(1.0+alpha*alpha*tan2));
}

float Distrib(float NH,float alpha2){
    float NH2 = clamp(NH*NH,0.0,1.0);
    float den = NH2*alpha2+(1.0-NH2);
    return alpha2 / (pi*den*den);
}

vec3 SpecularLight(vec3 N,vec3 L,vec3 V,vec3 H ,float a,float r,vec3 F){
    float NV = max(dot(N,V),0.0);
    if(NV == 0.0)return vec3(0.0);
    float NL = max(dot(N,L),0.0);
    float NH = max(dot(N,H),0.0);

    float r2 = r*r;
    float G = PGeom(NV,r2)*PGeom(NL,r2);
    float D = Distrib(NH,r2);

    return max(G*D*F*0.25/NV,0.0);
}

vec3 light(vec3 lpos,vec3 normal,vec3 vpos,vec3 pos,float a,float r,const vec3 F0){
    vec3 N = normal;
    vec3 L = normalize(lpos-pos);
    vec3 V = normalize(vpos-pos);

    vec3 H = normalize(L+V);

    //const vec3 F0 = vec3(0.5);//1.0-texture(diffuseTexture,uv).rgb;//vec3(1.0, 0.86, 0.56); // For testing
    float HV = max(dot(H,V),0.0);
    vec3 F = Fresnel(HV,F0);

    vec3 spec = SpecularLight(N,L,V,H,a,r,F);
    float diff = OrenNayer(N,L,V,a,r*r);

    return (spec+diff*clamp(1.0-F,0.0,1.0));
}