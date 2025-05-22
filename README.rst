Planetarium for Raspberry Pi Pico
==================================

.. image::  images/planetarium/thumbnail.png
  :target: https://youtu.be/9oXsnwg1oYE

The Pi Pico Planetarium is a compact, low-cost astronomy viewer built around
the Raspberry Pi Pico 2W and a 480x320 ST7796 TFT display. Designed with both
beginners and experienced hobbyists in mind, it offers a hands-on introduction
to microcontrollers, electronics, and astronomy, while remaining flexible and
expandable for more advanced use. The device displays a real-time map of the
night sky, including stars, constellations, planets, the Moon, and deep sky
objects. By using Wi-Fi to retrieve accurate time data via NTP, the planetarium
keeps the star chart aligned with the user’s current time and location.

Getting the Code
----------------

.. code::

  git clone https://github.com/dawsonjon/Pico-Planetarium


Documentation
-------------

For technical details refer to the `technical documentation <https://101-things.readthedocs.io/en/latest/planetarium.html>`__.

3D-Printed Enclosure
--------------------

A 3D printed enclosure can be found `here <https://github.com/dawsonjon/Pico-Planetarium/tree/main/enclosure>`__, including stl files and FreeCAD design files.


Install Arduino Pico
--------------------

The SSTV code is written in pure C++, but a demo application is provided as an `Arduino sketch <https://github.com/dawsonjon/Pico-Planetarium/tree/main/pico_planetarium>`__. The `Arduino Pico <https://github.com/earlephilhower/arduino-pico>`__ port by Earle Philhower is probably the easiest way to install and configure a C++ development environment for the Raspberry Pi Pico. Its possible to install the tool and get up-and running with example applications in just a few minutes. Refer to the `installation instructions <https://github.com/earlephilhower/arduino-pico?tab=readme-ov-file#installing-via-arduino-boards-manager>`__ and the `online documentation <https://arduino-pico.readthedocs.io/en/latest/>`__ to get started.


Credits
-------

This project uses the following libraries: 

+ `ILI934X display driver by Darren Horrocks. <https://github.com/bizzehdee/pico-libs/tree/master/src/common/ili934x>`__
+ `WiFi Manager Pico <https://github.com/mthorley/wifimanager-pico>`_


Data Sources
------------

+ `Bright Star Catalog <http://tdc-www.harvard.edu/catalogs/bsc5.html>`__
+ `Marc van der Sluys - Contellation Lines <https://github.com/MarcvdSluys/ConstellationLines?tab=readme-ov-file>`__
+ `Greg Miller - Constellation Lines <https://www.celestialprogramming.com/snippets/ConstellationCenterPoints/constellationCenterPoints.html>`__
+ `Greg Miller - Common Star Names <https://celestialprogramming.com/snippets/CommonStarNames.html>`__
+ `Eleanor Lutz - Mesier and NGC Objects <https://github.com/eleanorlutz/western_constellations_atlas_of_space/blob/main/data/processed/messier_ngc_processed.csv>`__


Other Links and References
--------------------------

+ `Greg Miller - Celestial Programming <https:celestialprogramming.com>`__
+ `Eleanor Lutz - Western Constellations Atlas of Space <https://github.com/eleanorlutz/western_constellations_atlas_of_space>`__
+ `Jet Propulsion Laboratory - Approximate Positions of the Planets <https://ssd.jpl.nasa.gov/txt/aprx_pos_planets.pdf>`__
