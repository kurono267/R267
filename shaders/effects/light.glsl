const float pi = 3.1415926535;

vec3 OrenNayer(vec3 normal,vec3 light,vec3 view,vec3 a,float r2){
	// Simple Lambert
	float LV = max(dot(light,view),0.01f);
	float LN = max(dot(light,normal),0.01f);
	float NV = max(dot(normal,view),0.01f);
	vec3 l = (a/pi)*LN;
	// Full
	// Compute A
	vec3 A = 1.0 + r2 * (a / (r2 + 0.13) + 0.5 / (r2 + 0.57));//1.0 - 0.5 / (r2 + 0.57);
	// Compute B
	float B = r2+0.09;
	B = (0.45*r2)/B;
	// Compute light
	float s = LV-LN*NV;
	float t = mix(1.0,max(LN,NV),step(0.0,s));
	l = l*(A+B*s/t);

	return l;
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
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
	return alpha2 / (pi*den*den+0.01f);
}

vec3 SpecularLight(vec3 N,vec3 L,vec3 V,vec3 H,float r,vec3 F){
	float NV = max(dot(N,V),0.01f);
	float NL = max(dot(N,L),0.01f);
	float NH = max(dot(N,H),0.01f);

	float r2 = r*r;
	float G = PGeom(NV,r2)*PGeom(NL,r2);
	float D = Distrib(NH,r2);

	return max(G*D*F*0.25/NV,0.0);
}

vec3 light(vec3 lpos,vec3 normal,vec3 vpos,vec3 pos,float metallic,float r,vec3 albedo,vec3 specColor){
	vec3 N = normal;
	vec3 L = normalize(lpos-pos);
	vec3 V = normalize(vpos-pos);

	vec3 H = normalize(L+V);

	vec3 F0 = vec3(0.04f);
    F0      = mix(F0, albedo, metallic);//1.0-texture(diffuseTexture,uv).rgb;//vec3(1.0, 0.86, 0.56); // For testing
	float HV = max(dot(H,V),0.0);
	vec3 F = fresnelSchlickRoughness(HV,F0,r);

	vec3 spec = SpecularLight(N,L,V,H,r,F);
	vec3 diff = OrenNayer(N,L,V,albedo,r*r);

	return (spec+diff*clamp((1.0f-F)*(1.0f-metallic),0.0,1.0));
}

vec3 ibl(vec3 N,vec3 vpos, vec3 pos, vec3 albedo, float metallic, float roughness, float lightScale){
	vec3 V = normalize(vpos-pos);

	vec3 F0 = vec3(0.04f);
    F0      = mix(F0, albedo, metallic);

    vec3 R = reflect(V, N);

	vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

	vec3 kS = F;
	vec3 kD = 1.0 - kS;
	kD *= 1.0 - metallic;

	vec3 irradiance = texture(irradianceMap, N).rgb*lightScale;
	vec3 diffuse    = irradiance * albedo;

	const float MAX_REFLECTION_LOD = 9.0;
	vec3 prefilteredColor = textureLod(background, R,  roughness * MAX_REFLECTION_LOD+1.0f).rgb*lightScale;
	vec2 envBRDF  = texture(brdf, vec2(max(dot(N, V), 0.0), 1.0f - roughness)).rg;
	vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);

	vec3 ambient = (kD * diffuse + specular);

	return ambient;
}