# LandscapeHDTool

Unreal Engine 5 editor plugin for creating high-resolution landscape actors from Landscape Streaming Proxies.

Select proxy actors in the viewport, choose a resolution multiplier, and the plugin will create new `ALandscape` actors with upscaled heightmap and weightmap data using bilinear interpolation.

## Features

- Upscale landscape resolution by 1x, 2x, 4x, 8x, 16x, or 32x
- Preserves heightmap data via bilinear interpolation
- Copies all weightmap layers (splat maps) to the new landscape
- Works with multiple selected proxies at once
- Displays detailed info about selected proxies (position, scale, resolution, layers)

## Installation

### Option 1: Clone into your project's Plugins folder

```
cd YourProject/Plugins
git clone https://github.com/cathxrsys/landscapehdtool.git LandscapeHDTool
```

### Option 2: Copy manually

1. Download or clone this repository
2. Copy the `LandscapeHDTool` folder into `YourProject/Plugins/`
3. Restart Unreal Engine

After installation, enable the plugin in **Edit > Plugins > Editor > LandscapeHDTool** if it is not enabled automatically.

## Usage

1. Open your level in the Unreal Editor
2. Select one or more `ALandscapeStreamingProxy` actors in the viewport or World Outliner
3. Click the **LandscapeHDTool** button in the toolbar (or go to **Window > LandscapeHDTool**)
4. Click **Refresh** to see info about the selected proxies
5. Click **Create Nx** (where N = 1, 2, 4, 8, 16, 32) to create a new high-resolution landscape

The new landscape will be created at the same location with proportionally scaled-down actor scale to maintain the same physical size at higher vertex density.

## How it works

The plugin reads heightmap and weightmap data from the selected `ALandscapeStreamingProxy` components, performs bilinear interpolation to the target resolution, and imports a new `ALandscape` actor with the upscaled data.

## Requirements

- Unreal Engine 5
- Editor-only plugin (not shipped in builds)

## Author

cathxrsys
