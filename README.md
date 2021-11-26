# Jole Dither
[![forthebadge](https://forthebadge.com/images/badges/0-percent-optimized.svg)](https://forthebadge.com)

# Blah Blah
I originally started writing this with png support but it quickly grew big, ugly and it didn't work quite right. I therefore switched to flabfeld, which is soo much more pleasant to work with. The endianess is backwards but it works out fine. In classic arch style it's not available in the repos but you can get it from the AUR.

# Usage
`jd` reads a farbfeld image on stdin and outputs a corrosponding dithered farbfeld on stdout.
```
2ff <cute.png | jd | ff2png >cuter.png
```
You can also override the palette:
```
echo -e cdbca8\n7b3f21\n441511\nccd3c9\n2e4b91\n700c08\nee4218\ne8c689\n040104\nb08464\na66b3e\n63321b\nc6b69d > pal
2ff <asuka.png | ./jd | ff2png >aska.png
```

