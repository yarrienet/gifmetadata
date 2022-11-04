# gifmetadata

CLI tool for extracting GIF metadata
	
## Overview

GIFs contain 'comments' that were commonly used to attribute copyright and attribution in the early days of the web. Since then programs have lost the ability to read and write this data. `gifmetadata` can read and output this data.

## Output

gifmetadata can read comments, application extensions and plain text embedded within a GIF. The GIF file format holds this information in "blocks".

### Blocks

|Block Name|Description|
|-|-|
|Comment|Text messages limited to 256 characters, primarily copyright and attribution messages.|
|Application extension|Custom extensions to GIFs that applications may use to add additional features to the GIF. For example Netscape 2.0 used them to add early animation looping. Application extensions contain a name and then 'sub-blocks' of binary data, this may ping your terminal.|
|Plain text|A feature within the 89a specification to display plain text on-top of images that was never utilized.|

## CLI

```
USAGE: gifmetadata [options] file

OPTIONS:

-h / --help      Display help, options and program info
-a / --all       Display all GIF metadata blocks instead of only the comment
-v / --verbose   Display more data about the gif, e.g. width/height
-d / --dev       Display inner program workings intended for developers
```