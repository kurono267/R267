//
// Created by kurono267 on 05.08.17.
//

#pragma once

#include <vector>
#include <string>
#include <base/scene/scene.hpp>

using namespace r267;

struct nk_context;

class Toolbar {
    public:
        Toolbar();
        ~Toolbar();

        void setScene(spScene scene);

        bool update(nk_context* ctx);
    protected:
        spMaterial _all;
        spScene    _scene;
        std::string              _selected;
        std::vector<std::string> _names;
};
