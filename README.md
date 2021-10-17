# Jole Dither
[![forthebadge](https://forthebadge.com/images/badges/0-percent-optimized.svg)](https://forthebadge.com)

# Blah Blah
I originally started writing this with png support but it quickly grew big, ugly and it didn't work quite right. I therefore switched to flabfeld, which is soo much more pleasant to work with. The endianess is backwards but it works out fine. In classic arch style it's not available in the repos but you can get it from the AUR.

# Usage
`jd` reads a farbfeld image on stdin and outputs a corrosponding dithered farbfeld on stdout.
```
2ff <cute.png | jd | ff2png >cuter.png
```
