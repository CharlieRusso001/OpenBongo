# Cat Pack System

This directory contains configurable cat packs for OpenBongo. Each cat pack is a folder with images and a configuration file.

## Creating a Cat Pack

1. Create a new folder in the `catpacks/` directory (e.g., `catpacks/MyCat/`)

2. Add your images:
   - Body image (required)
   - Hand up image (required)
   - Hand down image (required)
   - Icon image (optional, for the settings UI)

3. Create a `config.txt` file in your cat pack folder with the following format:

```
name=My Cat Name
bodyImage=body.png
handUpImage=handup.png
handDownImage=handdown.png
iconImage=icon.png

# Body offsets (relative to base position)
bodyOffsetX=0.0
bodyOffsetY=0.0
bodyOffsetZ=0.0

# Left arm offsets
leftArmOffsetX=0.0
leftArmOffsetY=0.0
leftArmOffsetZ=0.0
leftArmSpacing=1.1

# Right arm offsets
rightArmOffsetX=0.0
rightArmOffsetY=0.0
rightArmOffsetZ=0.0
rightArmSpacing=1.0

# Punch animation offset (multiplier for how high arms move)
punchOffsetY=0.3
```

## Configuration Options

- **name**: Display name of the cat (required)
- **bodyImage**: Filename of the body image (required)
- **handUpImage**: Filename of the hand up image (required)
- **handDownImage**: Filename of the hand down image (required)
- **iconImage**: Filename of the icon image (optional, shown in settings UI)

### Offset Values

All offset values are floats. Positive X moves right, positive Y moves down.

- **bodyOffsetX/Y/Z**: Offset for the body sprite position
- **leftArmOffsetX/Y/Z**: Offset for the left arm position
- **rightArmOffsetX/Y/Z**: Offset for the right arm position
- **leftArmSpacing**: Multiplier for left arm spacing from body (default: 1.1)
- **rightArmSpacing**: Multiplier for right arm spacing from body (default: 1.0)
- **punchOffsetY**: Multiplier for how high arms move when punching (default: 0.3)

## Example Structure

```
catpacks/
  MyCat/
    config.txt
    body.png
    handup.png
    handdown.png
    icon.png
  DevArt/
    config.txt
    body-devartcat.png
    handup-devartcat.png
    handdown-devartcat.png
```

## Notes

- Comments in config files start with `#`
- Empty lines are ignored
- Case-insensitive key matching
- If a config.txt is not found, the system will try to use default DevArt settings
- The selected cat pack is saved in `OpenBongo.catpack` file

