struct FSInput {
  [[vk::location(0)]] float2 UV : TEXCOORD0;
};

struct FSOutput {
  [[vk::location(0)]] float4 Color : SV_Target;
};

Texture2D uTexture: register(t0);
SamplerState uSampler: register(s0);

FSOutput main(FSInput input) {
	FSOutput output;
	output.Color    = float4(uTexture.Sample(uSampler, input.UV).rgb, 1.0f);
	// output.Color    = float4(input.UV, 0.0f, 1.0f);
	return output;
}