<div align="center">
	<h1>
	<img src="https://raw.githubusercontent.com/LMMS/artwork/master/Icon%20%26%20Mimetypes/lmms-64x64.svg" alt="LMMS Logo"><br>LMMS | Gentoo Patch
	</h1>
	<p>Music production software | Patched for Gentoo Linux</p>
	<p>
		<a href="https://lmms.io/">Official Website</a>
		⦁︎
		<a href="https://github.com/LMMS/lmms/releases">Official Releases</a>
		⦁︎
		<a href="https://github.com/LMMS/lmms/wiki">Official Developer wiki</a>
		⦁︎
		<a href="https://lmms.io/documentation">User manual</a>
		⦁︎
		<a href="https://lmms.io/showcase/">Showcase</a>
		⦁︎
		<a href="https://lmms.io/lsp/">Sharing platform</a>
	</p>
</div>

Why the fork of LMMS?
----------------------

Apart from compiling options and an install script, this is still fundamentally the same program. LMMS has an ebuild for Gentoo with broken LADSPA support. LMMS4Gentoo, as the name suggests, is not my work, nor am I claiming it as my own. It is simply a patch to ensure functionality on Gentoo Linux and other source based Distributions. The LMMS Devs also denied a simple installation script, so I put it here.

Changes from LMMS Master
------------------------
* Install script with support for open<b>doas</b> and <b>sudo</b>
* Automatic -DWANT_QT6=ON Flag (provided you run script)
* This very README (kinda obvious but worth mentioning)

Features
---------

* Song-Editor for arranging melodies, samples, patterns, and automation
* Pattern-Editor for creating beats and patterns
* An easy-to-use Piano-Roll for editing patterns and melodies
* A Mixer with unlimited mixer channels and arbitrary number of effects
* Many powerful instrument and effect-plugins out of the box
* Full user-defined track-based automation and computer-controlled automation sources
* Compatible with many standards such as SoundFont2, VST2 (instruments and effects), LADSPA, LV2, GUS Patches, and full MIDI support
* MIDI file importing and exporting

Building
---------

* Read the LMMS compiling Wiki to figure out deps
* Clone the repository to /tmp
* Enter the directory
* Add the execute mode to install.sh
* Run the install.sh script OR follow the LMMS wiki passing the <b>-DWANT_QT6=ON</b> flag on the cmake command 
