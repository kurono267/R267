//
// Created by kurono267 on 05.08.17.
//

#include "toolbar.hpp"
#include <base/gui/gui.hpp>

Toolbar::Toolbar(){
    _all = std::make_shared<Material>();
    _selected = "ALL";
}
Toolbar::~Toolbar(){}

void Toolbar::setScene(spScene scene){
    _scene = scene;
}

nk_color fromGLM(const glm::vec4& color,const bool isAlpha = false){
    nk_color result;
    result.r = color.r*255.0f;result.g = color.g*255.0f;result.b = color.b*255.0f;result.a = isAlpha?color.a*255.0f:255;
    return result;
}

glm::vec3 rgbColorPicker(nk_context* ctx, const glm::vec4& color, const bool isAlpha = false){
    nk_color nColor = fromGLM(color,isAlpha);
    if (nk_combo_begin_color(ctx, nColor, nk_vec2(nk_widget_width(ctx),400))) {
        nk_layout_row_dynamic(ctx, 120, 1);
        nColor = nk_color_picker(ctx, nColor, isAlpha?NK_RGBA:NK_RGB);
        nk_layout_row_dynamic(ctx, 25, 1);
            nColor.r = (nk_byte)nk_propertyi(ctx, "#R:", 0, nColor.r, 255, 1,1);
            nColor.g = (nk_byte)nk_propertyi(ctx, "#G:", 0, nColor.g, 255, 1,1);
            nColor.b = (nk_byte)nk_propertyi(ctx, "#B:", 0, nColor.b, 255, 1,1);
        if(isAlpha)
            nColor.a = (nk_byte)nk_propertyi(ctx, "#A:", 0, nColor.a, 255, 1,1);
        nk_combo_end(ctx);
    }
    return glm::vec3((float)nColor.r/255.0f,(float)nColor.g/255.0f,(float)nColor.b/255.0f);
}

bool Toolbar::update(nk_context* ctx){
    if (nk_begin(ctx, "Toolbar", nk_rect(0, 0, 250, 250),
                 NK_WINDOW_BORDER| NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE)) {
        if(_scene) {
            bool isAll = _selected == "ALL";

            nk_layout_row_dynamic(ctx, 20, 1);
            nk_label(ctx, "MATERIAL", NK_TEXT_CENTERED);
            nk_layout_row_dynamic(ctx, 20, 1);
            if (nk_combo_begin_label(ctx, isAll?"ALL":_selected.c_str(), nk_vec2(nk_widget_width(ctx), 400))) {
                nk_layout_row_dynamic(ctx, 20, 1);
                if (nk_combo_item_label(ctx, "ALL", NK_TEXT_LEFT))
                    _selected = "ALL";
                else {
	                for (auto m : _scene->materials()) {
	                    if (nk_combo_item_label(ctx, m.first.c_str(), NK_TEXT_LEFT))
	                        _selected = m.first;
	                }
                }
                nk_combo_end(ctx);
            }
            isAll = _selected == "ALL";
            auto mat = isAll?_all:_scene->materials()[_selected];
            glm::vec3 diffuseColor = glm::vec3(0.0f);
            if(!isAll){
	            nk_layout_row_begin(ctx, NK_STATIC, 20, 2);
	            {
	                nk_layout_row_push(ctx,50.0f);
	                nk_label(ctx, "Albedo:", NK_TEXT_LEFT);
	                nk_layout_row_push(ctx,165.0f);
	                diffuseColor = rgbColorPicker(ctx,mat->data().diffuseColor);
	            }
            }
            mat->setDiffuseColor(diffuseColor);
            nk_layout_row_dynamic(ctx, 20, 1);
            float rough = mat->data().specularColor.w;
            nk_property_float(ctx,"Roughness",0.01f,&rough,1.0f,0.01f,0.01f);
            mat->setRoughness(rough);
            float metallic = mat->data().diffuseColor.w;
            nk_property_float(ctx,"Metallic",0.01f,&metallic,1.0f,0.01f,0.01f);
            mat->setAlbedo(metallic);
            /*if(isAll){
                for (auto m : _scene->materials()) {
                    m.second->setRoughness(rough);
                    m.second->setAlbedo(metallic);
                }
            }*/
            nk_layout_row_dynamic(ctx, 20, 1);
            int save = nk_button_label(ctx, "Save");
            if(save){
                _scene->save(_scene->getFilename());
            }
        }
    }
    nk_end(ctx);
    return true;
}