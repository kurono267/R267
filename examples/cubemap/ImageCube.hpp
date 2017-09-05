//
// Created by kurono267 on 04.09.17.
//

#pragma once

class ImageCube {
	public:
		ImageCube(){}
		~ImageCube(){}

		void init(spDevice device,spImage source);

	protected:
		spPipeline _pipeline;
		spImage    _source;
		spImage    _cubemap;
};
