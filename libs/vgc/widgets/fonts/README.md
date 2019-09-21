We use the following fonts:

- Source Sans Pro, OTF, 3.006
  https://github.com/adobe-fonts/source-sans-pro/releases/tag/3.006R

- Source Sans Pro, TTF, 2.021-ro-1.076-it
  https://fonts.google.com/specimen/Source+Sans+Pro?selection.family=Source+Code+Pro:200,200i,300,300i,400,400i,500,500i,600,600i,700,700i,900,900i

- Source Code Pro, OTF, 2.030-ro-1.050-it
  https://github.com/adobe-fonts/source-code-pro/releases/tag/2.030R-ro%2F1.050R-it

- Source Code Pro, TTF, 2.030-ro-1.050-it
  https://fonts.google.com/specimen/Source+Code+Pro?selection.family=Source+Code+Pro:200,200i,300,300i,400,400i,500,500i,600,600i,700,700i,900,900i

We currently use the TTF fonts on all platforms, but keep the OTF for
now, until we've tested on all desktop and mobile platforms.

In the case of Source Sans Pro, we use older TTF versions hosted on Google
Fonts because the TTF versions hosted on the Adobe GitHub seem not to be
hinted properly and as a result don't look good on Windows 10. The lack of
hinting is clearly stated since 2.040-ro-1.090-it, but earlier versions don't
seem hinted properly either (tested with 2.020-ro-1.075-it).

Source Code Pro seems properly hinted both using the Adobe version and the
Google version, which are actually based on the same version at the time of
writing (2.030-ro-1.050-it). However, the Adobe version, both the OTF and
the TTF, doesn't render properly on macOS: the font stays black instead
of the intended color. This is why we use the Google version.

For more info on tests between TTF, OTF, Google, and Adobe versions, see:

https://github.com/vgc/vgc/issues/173
