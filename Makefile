
ROOT  = .
BUILD = ./build

.DEFAULT_GOAL := all

SPV_SRC = $(ROOT)/src/shaders
SPV_DST = $(BUILD)/data/shaders
SPV_VERT = $(addprefix $(SPV_DST)/,$(addsuffix .vert.spv,generic font basic2d mesh terrain))
SPV_FRAG = $(addprefix $(SPV_DST)/,$(addsuffix .frag.spv,generic font basic2d mesh terrain))

$(SPV_DST)/%.vert.spv: $(SPV_SRC)/%.glsl
	glslangValidator -V -g -S vert -DVERTEX_SHADER $< -o $@

$(SPV_DST)/%.frag.spv: $(SPV_SRC)/%.glsl
	glslangValidator -V -g -S frag -DFRAGMENT_SHADER $< -o $@

shaders: | $(SPV_DST) $(SPV_VERT) $(SPV_FRAG)


FONT_SRC = $(ROOT)/assets/fonts
FONT_DST = $(BUILD)/data/fonts
FONTS = $(addprefix $(FONT_DST)/,Roboto-Regular.ttf)

$(FONT_DST)/%.ttf: $(FONT_SRC)/%.ttf
	cp $< $@

fonts: | $(FONT_DST) $(FONTS)


MODELS_SRC = $(ROOT)/assets/models
MODELS_DST = $(BUILD)/data/models
MODELS = $(addprefix $(MODELS_DST)/,cube.obj armoire.obj)

$(MODELS_DST)/%.obj: $(MODELS_SRC)/%.obj
	cp $< $@

models: | $(MODELS_DST) $(MODELS)


TEXTURES_SRC = $(ROOT)/assets/textures
TEXTURES_DST = $(BUILD)/data/textures
TEXTURES = $(addprefix $(TEXTURES_DST)/,dummy.bmp player.bmp terrain.bmp)

$(TEXTURES_DST)/%.bmp: $(TEXTURES_SRC)/%.bmp
	cp $< $@

$(TEXTURES_DST)/%.r16: $(TEXTURES_SRC)/%.r16
	cp $< $@

textures: | $(TEXTURES_DST) $(TEXTURES)


FLAGS = -std=c++11 -g

NOWARNINGS = -Wno-int-to-void-pointer-cast -Wno-nested-anon-types -Wno-unused-function
WARNINGS   = -Wall -Wextra -Wpedantic $(NOWARNINGS)

INCLUDE_DIR = -I$(ROOT)/src -I$(ROOT)

OPTIMIZED   = -O3
UNOPTIMIZED = -O0

LEARY_LIBS  = -lvulkan -lX11 -lXi -ldl
LEARY_FLAGS = $(FLAGS) $(WARNINGS) $(UNOPTIMIZED) $(INCLUDE_DIR)

$(BUILD)/game.so: FORCE
	$(CXX) $(LEARY_FLAGS) -fPIC -shared $(ROOT)/src/platform/linux_leary.cpp -o $@

$(BUILD)/leary: FORCE
	$(CXX) $(LEARY_FLAGS) $(LEARY_LIBS) -o $@ $(ROOT)/src/platform/linux_main.cpp

leary: $(BUILD)/game.so $(BUILD)/leary


TOOLS_FLAGS = $(FLAGS) $(WARNINGS) $(UNOPTIMIZED) $(INCLUDE_DIR)
$(BUILD)/preprocessor: FORCE
	$(CXX) $(TOOLS_FLAGS) $(ROOT)/tools/preprocessor.cpp -o $@

$(BUILD)/benchmark: FORCE
	$(CXX) $(TOOLS_FLAGS) -O2 $(ROOT)/tools/benchmark.cpp -o $@

tools: $(BUILD)/preprocessor


all: shaders fonts models tools textures leary

$(SPV_DST):
	mkdir -p $(SPV_DST)

$(FONT_DST):
	mkdir -p $(FONT_DST)

$(MODELS_DST):
	mkdir -p $(MODELS_DST)

$(TEXTURES_DST):
	mkdir -p $(TEXTURES_DST)

FORCE:
