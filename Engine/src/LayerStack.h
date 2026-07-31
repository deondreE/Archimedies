#pragma once
#include "Layer.h"

namespace Engine {


	class LayerStack {
	public:
		LayerStack() = default;
		~LayerStack();

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);
		void PopLayer(Layer* layer);
		void PopOverlay(Layer* overlay);

		std::vector<Layer*>::iterator begin() { return _Layers.begin(); }
		std::vector<Layer*>::iterator end() { return _Layers.end(); }
		std::vector<Layer*>::reverse_iterator rbegin() { return _Layers.rbegin(); }
		std::vector<Layer*>::reverse_iterator rend() { return _Layers.rend(); }

	private:
		std::vector<Layer*> _Layers; 
		unsigned int _LayerInsertIndex = 0; // regular layers go before this index, overlays after
	};
}