# STR-X

## Custom Digital Guitar Amp

### About

STR-X is a guitar amp designed from the ground-up to be a fully digital guitar amp. No attempt at analog modelling was made here--rather, the goal was to use simple DSP to create a unique-sounding guitar amp for the era of DAWs and making music on a laptop (you can use a desktop, of course, if you're a nerd).

The result was *indeed* a very unique-sounding guitar amp. Clearly geared toward hi-gain tones, users seemed to find it worked best for a sludgy, doom metal tone, or for very thick hard rock guitars. Nonetheless, it can be surprisingly versatile with some massaging. But don't expect "vintage tube warmth" or "British bite" or whatever. It's not your grandpa's guitar amp.

### Installation

#### MacOS

If you're on a Mac, simply run the .pkg file you downloaded and follow the instructions. You can customize which versions of the plugin you need to install.

#### Windows

Windows users...I'm sorry.

I was brand new to making software when I first made the STR-X. As a result, the naming and organization of each plugin version has been a mess. The nomenclature up to this point has been as follows:

##### v1.0:

STR-X.vst3/dll (also sorted under company name "Strix Audio")

##### v1.1:

STR-X_v1.1.vst3/dll

With the plugin showing up in your DAW as STR-X or STR-X(v1.1). From now on, the plugin will only be named STR-X, and it will overwrite itself with each new update. But from v1.1 to v1.2, it won't actually  overwrite. You can manually add STR-X v1.2 to any old projects that use v1.1.

#### Linux

Just run the install script and you'll be good to go! The script allows you to pick each format as you go and lets you choose a new directory if you wish. Or, you can just manually copy the plugin files to whichever folder you want.

### The Controls

#### Hi Gain

This will toggle between the Low- and High-gain channels. Low-gain is new to v1.2, and features a new saturation algorithm. Better for punchier, more intelligible tones, it gives your guitar a bit more breathing room, but it also digs in early--still not suitable for clean tones!

#### Mode

This three-way menu selects the crossover point used in the preamp section.

***Normal*** will give you a solid low-end, with retained clarity in the mids and highs even when cranked.

***Thick*** drops the crossover point lower, resulting in greater intermodulation of the distorted signal, and a fuzzy tone ready for a sludge or doom sound.

Finally, ***Open*** separates all of the low-end from the mids and highs for a tight and bright tone, perfect for fast palm-muted playing or any situation where the low-end of the other two modes might be excessive.

#### X

X is a one-knob distortion pedal, based roughly around the type of midrange focusing you get from a Tube Screamer. No, that doesn't mean we modeled a Tube Screamer. We meant what we said earlier (and not just for legal reasons).

#### Gain

This acts as the input preamp gain for the amp head. This is where the filter crossover happens, and controls a great deal of how the tone is colored. Both the low and high frequency bands go through the preamp stage independently, resulting in interesting distortion shapes and colors, depending on the mode selected, which brings us to

#### Tone Controls

Pretty much what you'd expect! If you've used a guitar amp, and EQ, or anything pertaining to sound equalization before, you know what this section does.

There's also an option to use the v1.0 tone controls, perhaps if you want to maintain backwards compatibility with an old project (people *did* download the first version...)

Essentially, the legacy tone controls offer less range, and the ranges aren't symmetric--i.e. some of them boost more than they cut, etc. Go wild.

#### Power Amp

Previously named "Master" until it was brought to my attention that such a name implies a fully linear--i.e. non-distorting--volume control, this knob functions like the power amp on a guitar amp head, which can be highly nonlinear and distorting in character.

You should treat this like another distortion knob, but one that generates the type of harmonics you'd expect from Class-B power amp tubes in a guitar amp (no, we did not model pentodes for this plugin). *Don't* treat this like a clean output gain. For that you'd want:

#### Output Volume

The real, distortion free master volume, allowing you to set the final level of the STR-X before it goes into the next plugin in your chain, or your channel fader.

#### HQ

Activates 4x oversampling with minimum-phase filters.

If that means nothing to you, don't worry. I had to put this in to satiate the nerds on certain audio forums. Basically, there are certain things about digital distortion that are a little less-than-ideal sounding, if pushed to an extreme. Enabling this *does theoretically* get you a more analog-sounding response. But at what cost?

Only introduces 4 samples of latency, but increases CPU use by a good chunk. If you find the aliasing of the STR-X to be an issue, this is your button.

#### Render HQ

Defers 4x, linear-phase oversampling until you're rendering a track, song, or what have you. If the extra CPU from HQ mode is too much for your ancient computer to handle, consider trying this out. It may slow down your renders, but you'll get less aliasing in the tone, and you can play in real-time without putting your computer down for good.

Additionally, this works well in conjunction with HQ, since the filters are linear-phase. So you can use the low-latency filters while recording and mixing, and then when it's time to render this will automatically use linear-phase filters for a more accurate high-frequency response.

### CHANGES

#### v1.2

//==================

- New "Low-Gain" amp mode
- CLAP plugin format support with non-destructive parameter modulation
- Linux support (including LV2)
- Stereo in/out support
- Improved parameter smoothing
- GUI refactor and new theme for Low-Gain mode
- General optimization and code cleanup

#### v1.1

//==================

- macOS support (Catalina/Big Sur/M1)

- 4x oversampling with minimal-latency, polyphase IIR filters, with the option for offline render-only 4x oversampling with linear-phase FIR filters

- "De-kinked" preamp waveshaping transfer curve for more accurate asymmetrical distortion and less aliasing

- Expanded range of tone controls

- Allow for using v1.0 tone controls with "Use Legacy Tone Controls" option

- Fully resizable GUI (VST3 and AU only) [also implemented poorly and didn't have a resize handle yet]

### Credits

Utilizes the Class B valve waveshaper as described in Will Pirkle's book, "Designing Audio Effect Plugins in C++: Addendum A19 Vacuum Tube & Distortion Emulation Part 2"
This waveshaper is taken from a patent application (abandoned as a patent) by Mark Poletti: "US 2008/0049950 Nonlinear Processor for Audio Signals"

Additionally, this plugin uses code for converting parameters from knob values to processing values, taken from Will Pirkle's fxobjects header file,
available at https://www.willpirkle.com/Downloads/fxobjects_1.1.zip

Special thanks to Will Pirkle, Joshua Hodge of The Audio Programmer, and the JUCE Team.
