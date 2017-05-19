#pragma once
// Kurono267 Simple mesh format
namespace r267 {

enum format {
	R267Magic = 0x26700000,
	R267Mesh  = 0x26701000,
	R267VB    = 0x26701001,
	R267IB    = 0x26701002
};

} // r267
