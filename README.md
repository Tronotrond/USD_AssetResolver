# USD_AssetResolver
Quick and easy USD asset resolver; path remapper


Created this as a hands off approach to USD path remapping.
Ideal for mixed Windows and Linux pipelines. Can be set up on the farm to handle path remapping without the need for user interactions.

The config json defines easy from -> to path remapping rules.
The plugin will remap internal USD paths at runtime.


Compile and copy libSimpleMapResolver.so, json config file and plugInfo.json to a directory of your choosing.

To work with renderes like SideFX Houdini HUSK, set these environment variables



PXR_PLUGINPATH_NAME=/opt/usd_plugins/SimpleMapResolver

PXR_AR_DEFAULT_RESOLVER=SimpleMapResolver

SIMPLE_RESOLVER_CONFIG=/opt/usd_plugins/SimpleMapResolver/simple_resolver.json


Change the paths to match your installation directory.
