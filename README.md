# Noodles Pre-Fortress
This is my personal fork of PF2 from my copy of the code dated April 6th, 2022. The intention was to have the 0.7 version of the game be open source, as was 0.6, but I suppose they got cold feet. I feel it has been long enough now where I feel comfortable releasing this without impacting the current project, in spite of how little the game has noticeably changed since I left in 2022.

My intention with this is to finish everything I wanted to finish, plus anything else I feel like. This is being tested on the 0.7 build of the game, the download can be found on the official site [here](https://dl.prefortress.com/0.7/pf2_0.7.0.7z)

This is not endorsed by any current developer, but is endorsed by me, who did a significant amount of the code. The original code is based on a leaked 2008 version of the TF2 source code. This shouldn't really be an issue since almost all functionality is present in the public SDK, and anything that isn't is probably insignificant.

# Planned features
## Actual things
- ~Fix jittery control points~ - DONE
- 2fort with sewer water at different heights that's not awful
	- Problem is that it uses the heigher water height to determine the clipping of the view plane. I can fix it by breaking visibility in a strange way, but it's not very reliable so I need something a bit smarter and more controlled. Might just have to manually edit the water vis leaves.
- Implement grappling hook and Mannpower power ups
  - I originally wrote a grappling hook implementation but they never used it, and now I have the modern TF2 code to reference
- Replace building hauling with dismantling
  - Unsure how I want this to work yet. Currently the player would be able to right click and delete the building, picking up some or all of the metal used to build it, and stored in it for the dispenser. The player would move slower based on how much metal they have. The overdrawn metal would build/upgrade much faster. Intention with this would be to allow the engineer to quickly pack up his nest in the event of a big push/grenade spam, while also being more like TFC.
- Replace flame rocket with shorter range projectile
  - I think the current flame rocket is exceptionally annoying, having something more like the dragon's fury with longer range sounds preferable. 
- ~Restore scout double jump~ - DONE
- Recompile full lambert skin shader for current SDK
- Port various leaked maps
- Fix grenade viewmodel animation jittering
- Make spawn room turret not a building object
  - No reason for this other than some animation bug I didn't fix at the time
- Flag throw only on death
- Better indiciation of health saved with armor
  - Armor can be a bit confusing so showing the damage you took, then the damage saved from the armor would help clarify what exactly it does

## Fuzzy ideas
- Figure out an interesting solution for the hallucination grenade
  - The view distortion is currently just a half meassure so the grenade wasn't totally useless. A rough implementation was made at some point, but I didn't think it was very good or convincing. Any implementation probably has to be 70-80% functional to even determine if it's viable.
- Allow heavy greater grenade jumping without it being horrifically overpowered
  - Heavy's blast jumping was severly nerfed for obvious reasons. I would like to boost it a bit so it's something more viable, but still slightly nerf heavy while doing it. Maybe increase rev/firing delay while airbourne and slightly after.
- New main menu
  - Want to reattempt the WON style menu I did for 0.5 or 0.6? But less shitty
- Possible to speed up loading times?
- Port to new TF2 SDK?
  - Unlikely as I would lose the benefit of a smaller cleaner and faster codebase. Might back port somethings like newer map entities and some vscript stuff.