LONG-TERM TO DO:

* Energetic model - distinguish between a bee using nectar for its own energy needs versus collecting nectar for the hive?

* Forage nearest flower - once the bee has selected a nearby flower as a target, it will head for that flower with a probability of 90% (previous was 100%), and 10% of the time it will just head in a random direction. Can we find real-world data to justify this 90/10 split?

* Do we have real-world data on what bees do when they hit an obstacle? Previously, if a bee hit a wall (or the perimeter of the environment), I nudged it a little bit away from the wall but did not change its direction, So on the next step it was liable to head towards the wall again although it would probably turn a little bit left or right, but essentially it was likely it would bang into the wall a few times before turning enough to fly off unobstructed. I've now changed the code so that when it hits a wall it turns straight away to head parallel to the wall, so it's less likely to repeatedly bang into it. Does that sound sensible? It seems like a sensible thing to do, but on the other hand the repeated banging against the wall of the original implementation actually looked quite like what real bees might do!

---

## optimum bee density

Alan Dorin
10 Dec 2025, 04:09 (9 days ago)
to me, Hazel

Hi Tim (CC Hazel)

…this article is worth noting for future reference:
https://besjournals.onlinelibrary.wiley.com/doi/full/10.1111/1365-2664.13355

Specifically, it is a meta-study that concludes that there’s an optimum number of bee visits (we knew this for strawberries for instance) and that too many visits is detrimental, as are too few. So we have reason to want to do more with our simulations than saturate an area with bees… sometimes we’ll need to understand how to *reduce* bee numbers and to then manage the configuration so that the total number of visits within a specific window of flowering is expected to be within a range of values, e.g. "5-6 visits expected during a two week period". Very tricky!

Alan

---

## RominaRader-protective covering bee.pdf

Alan Dorin email to t@tt - 19/12/25
Subject: RominaRader-protective covering bee.pdf

Hi Malika and Tim,

I just recalled this attached paper (I had forgotten it briefly) from Romina. A part of the experiments here assessed the impact on bee movement of hail and bird netting. This is what we need for our simulations Tim. I’m extracting (some of) relevant sections below so it’s clearly in our email chain. Malika, we may not have to do further experiments on hail / bird net hole size, maybe we could just test netting colour’s impact.

:-)
Alan

2.2. Netted enclosures

Netted enclosures were already established on the farms and were constructed of wooden poles approximately 4–6 m high over which anti-bird netting was stretched to create a taut canopy, extending down the sides (Fig. 1a). Entry into the enclosure was through a removable section of anti-bird netting. The enclosure space varied between 22744 and 318732 m3 (Table A.3) and was located on sloping ground. Anti-hail netting had been placed on top of three enclosures, in the place of anti-bird netting, which resulted in anti-hail netting on the top of those enclosures and anti-bird netting on the sides (Table A.1). Anti-bird and anti-hail netting was comprised of black or white nylon mesh with 18 ×18 mm and ≤ 8 ×8 mm holes, respectively in a grid pattern (Fig. 1b; 1c).

2.4. Sampling method

Honey bee access to outside resources was assessed by visual observation of individual honey bee foragers exiting the hive and immediately attempting to leave the netted area (i.e. nets at the top or the side of the enclosure). Upon the forager arrival at the net, the time of exit for each bee was recorded using a stopwatch. All net rebounds were recorded until the bee was either lost from sight (within the crop or simply lost), successfully passed through the net, or abandoned the exit attempt.

2.5 […]

We examined the effect of netting on bee ability to exit the netted enclosures by conducting three separate GLMMs using the response variables number of rebounds, exit time (seconds), and exit success (yes or no). The same fixed effect (net type) was used for all three models. Variation due to location in the response metrics were accounted for by nesting the farm location as a random effect. Post-hoc comparisons were undertaken using Tukey’s comparisons via the emmeans package (Lenth et al., 2023).

3. Results 3.1. Access to outside resources Forager bees attempting to leave the netted area took significantly longer (estimate = 0.43, Standard Error (SE) = 0.11, p = 0.0001) to pass through anti-hail net (Mean (M) = 12.65, SE = 1.11 sec) than through anti-bird net (M = 9.08, SE = 0.56 sec) (Fig. 3; Appendix Output 1). Bees contacted and rebounded from the net on significantly more occasions (estimate = 0.76, SE = 0.16, p = <0.0001) when exiting anti-hail netting (M = 8.98, SE = 1.31) when compared to anti-bird netting (M = 4.60, SE = 0.38) (Fig. 3; Appendix Output 1). Bee success in exiting anti-hail netting (M = 31.46, SE = 0.05 %) was significantly less (estimate = 0.35, SE = 0.12, p = 0.004) than bee success in exiting the anti-bird netting (M = 50.73, SE = 0.03 %) (Fig. 2; Appendix Output 1). These results indicate a trend of increased time and attempts, with decreased exit success with anti-hail netting, when compared to anti-bird netting.

