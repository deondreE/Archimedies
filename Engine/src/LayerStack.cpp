#include "archpch.h"
#include "LayerStack.h"

namespace Engine {

	LayerStack::~LayerStack() {
		for (Layer* layer : _Layers) {
			layer->OnDetach();
			delete layer;
		}
	}

	void LayerStack::PushLayer(Layer* layer) {
		_Layers.emplace(_Layers.begin() + _LayerInsertIndex, layer);
		_LayerInsertIndex++;
		layer->OnAttach();
	}

	void LayerStack::PushOverlay(Layer* overlay) {
		_Layers.emplace_back(overlay);  // overlays always go at the very end (rendered last = drawn on top)
		overlay->OnAttach();
	}

	void LayerStack::PopLayer(Layer* layer) {
		auto it = std::find(_Layers.begin(), _Layers.begin() + _LayerInsertIndex, layer);
		if (it != _Layers.begin() + _LayerInsertIndex) {
			layer->OnDetach();
			_Layers.erase(it);
			_LayerInsertIndex--;
		}
	}

	void LayerStack::PopOverlay(Layer* overlay) {
		auto it = std::find(_Layers.begin(), _Layers.begin() + _LayerInsertIndex, overlay);
		if (it != _Layers.end()) {
			overlay->OnDetach();
			_Layers.erase(it);
		}
	}
}