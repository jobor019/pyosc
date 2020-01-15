MaxOsc is a package designed to conveniently call python code from MaxMSP over OSC. It's a wrapper of the [python-osc](https://pypi.org/project/python-osc/) package with a built-in parser.

The python code should be run independently of Max (in a terminal or similar) and can be used in conjuncture with the set of Max abstractions available [here](https://github.com/jobor019/pyosc/tree/master/pyosc) or directly through the [CNMAT externals](https://cnmat.berkeley.edu/downloads) for OSC (also available through the Max package manager).

### Project Status

The project is currently in development.

Documentation is currently limited, further documentation will be available later.



### Minimal Example
Run the following code in a separate terminal. Note that `MaxOsc.run()` will instantiate a [blocking OSC server](https://python-osc.readthedocs.io/en/latest/server.html#blocking-server), hence no code below that point will ever be executed.
```python
from maxosc import MaxOsc

class Flipper(MaxOsc):

    def __init__(self):
        super().__init__()
        self.run()

    def flip(self, a1: int, a2: int):
        return a2, a1

    def flip_list(self, a: [int]):
        return list(reversed(a))

if __name__ == '__main__':
    Flipper()

```

If the [pyosc abstractions](https://github.com/jobor019/pyosc/tree/master/pyosc) are available in your path, the following Max patcher can be used:
<img src="https://raw.githubusercontent.com/jobor019/pyosc/master/maxosc/docs/misc/readme1.png" width="50%">

<details><summary>Show Max patcher</summary>

```
<pre><code>
----------begin_max5_patcher----------
432.3ocwU9saBBCEF+Z3onoWyL.BLcWs2ikESEqtZv1l1hhw369n+AmNItlg
QuvZ5omxW+w2oGNDF.myZvRH3MvGfffCgAAlP5.At4AvMnlxJjzjFjh2wluF
FYWRgaTlv78LY4nJhTsSf3cKSq2vpUUXkYuItnjElsz9XdI40tTs4o1yw1SC
DB9zsDGoJ+hPWMSfKU1UGmlNJNBLMWONISOpibZKsBSnmzUG6XXndHZfXJHT
E3cNiWyAI8iYbOXV.+SVRiiMHkb+gYCVJQqvWQSBHELFjAx82uxGjeUXPLO+
1Hl978q3a3WIYOQ+xamZ5+ynbvYMpzoixeTFko+wxZZIXYEgOS2IoeVytl0a
zDI5redTfl39Kt3w0R4Rx8F5hgCsyrcPOI2elMY.qHze+wCiJ53W9hPxpEkc
mutF+feDZAVpHTjhvnmkjKmdeW6sP4dHTx8PIeHR+gfAKTgOBEeGDZpGBcMP
1ZCDmuEKjtjMRzdcXMSnmNIxLkPsSMsufB7VRW91HHQaEqpsbsVXNVvlhLnc
qrEXAsl3pKagqURyUMJps6IGY4vbiL7X32.J7svT
-----------end_max5_patcher-----------
</code></pre>
```
</details>


### Elaborate Example
##### Python Code:
```python
from maxosc import MaxOsc


class Instrument:
    def __init__(self, name: str, midi_range: (int, int), supported_techniques: [str], concert_pitch: float,
                 transposing: bool):
        self.name: str = name
        self.midi_range: (int, int) = midi_range
        self.supported_techniques: [str] = supported_techniques
        self.concert_pitch: float = concert_pitch
        self.transposing: bool = transposing

    def max_getter(self):
        return self.name, self.midi_range, self.supported_techniques, self.concert_pitch, self.transposing


class InstrumentBank(MaxOsc):
    def __init__(self):
        super().__init__()
        self.instruments: [Instrument] = []
        self.run()

    def add_instrument(self, name: str, midi_range: (int, int), supported_techniques: [str],
                       concert_pitch: float = 440.0, transposing: bool = False):
        instrument = Instrument(name, midi_range, supported_techniques, concert_pitch, transposing)
        self.instruments.append(instrument)
        print(f"Added instrument '{name}' with range={midi_range}, techniques={supported_techniques}, "
              f"concert pitch={concert_pitch} and transposing={transposing}.")

    def get_instruments(self):
        return [instr.max_getter() for instr in self.instruments]


if __name__ == '__main__':
    InstrumentBank()

```

##### Max Code:
<img src="https://raw.githubusercontent.com/jobor019/pyosc/master/maxosc/docs/misc/readme2.png" width="95%">
<details><summary>Show Max patcher</summary>
<pre><code>
----------begin_max5_patcher----------
1581.3ocyZF0bahCDG+YmOE6vKocFWeHDfgNSd35C2SW6K2My8PhmLXrrM4v
HJRjX2N86dWIgcH01Dpwl1YRLFYEzt+j1c+K470qFXMkulIrf2C2BCF70qFL
P2jpgAU2OvZUz53zHgtaVw7UqXYRqglOSxVK0s+wxTYx6dhWLCDxhjrEBPf8
Cjb3iQqgmRRSgoLHOpPvlAQBcqhMqlxSEPxH1HXFubZJC9bIWFIS3Yvpnh+W
.uQTN8AVr9IEuLJaA6si1N5oIYrXdYl1DbqZLqbEuTlxjZC1tp0jYZyjO8g2
E5s8uOORFuDs06KvAvPA5X+Q1CApuq5BwOPcw2djML44meR11GOQ012t5J0K
C6HB+DOigTfIxtVBr0IBIjjo3zH3+1Ce2g1QZ5cVWB9PZfOiscqyGGR+wmOv
4ornLAxn8IztOr4Uc1.u.HW.nED1.z777pCMZOtn5CafYr4QXz4PHiIjHFRQ
tI1wl4oQRIKCa+okrr53aD7OLFFEtdIKMGlifaEufgDGe6JSHJ9ihsn2BlFE
cEiia.ijPZcLNdb+Fa9dnTvTAd36UAdwQBEMPhIRjIOxd6AccZqc8lRKE3ZR
KENxaHXbb2w8YP2VmWVTpbdbwvcVyiREWDRPaJADw4YRPrI8GJ9yhEkpO.xh
VwDXfwFU7inLOOMACeRv+pYIQRV5FLXHMk+D13zMHmt4NqQv+tD6adTLChlK
YEllgDAvyUgRQocjYMkz1MztFyB6QlkugKhGox37TQTNDGkoXVAKOEIAlxIQ
tDsuICAF9NjJ5F3lqnCaHlnanYbSoloNg0PC0t+PyeqRC+Gxxb0C2Dd8RZcP
u1o0dcSYR88pGCYRsP6CmdeYhFOWUBYewecj.MkPk35ncdSdUBkd9YvzRojm
YcPykb.ysVVuBLCClj3dVVDpItt+YdFxM4LieXMEUuXsynOToCmZA+N5RmpW
+04mNWF+zOv62K+jbY7SWi7mee7S6Kie5DFn7vKmedrbT+UYVrJGjXWUnHck
eUmeOfYsVrXakJsSzojT9AMklVKzyIPq3gXt7KMGku+kYtl5WyQuDy0nfMQz
B1dy0W+ItR3JppUM6dM7Fk3dP8xagaUh9mzn1i7BlZCS5RV22hE.GBotVGCa
sIsdnFYUR5BOFxbNmHC6BV4lo268GldsZ+s1PnGLAtcd5WlfQH39Ex4pR52n
1tvEmfzSifUELLDjPr6OD9XBGcUXUxrj6KTm9vMvsddnRrwSz6mfWf6S+dIK
dYVxmKYhatMpHlC4Ie4Kv0w7THksHie8ot1j1ZxRNMxVUhphrlC+nWIqAlvy
T6NqcXC20kqq8HvV03KVJiehYkLtL1zGB1mWjg.6h5b2p8h4YcvYBu1xYO6S
iylzlTOiFVOMtUme44iyYrmPC7HaqaNVnDVvj2+LeDGtTh699rq2w84g09sE
9OIn15LGm9ntwAiGacYTWmSa9l3SUhgL6lmXNHji6umy3JS.U6cvSbAs4v2M
9mqW+4dlzFs18nAcIds5vX7O+t2OoZN5EUM2kS3diIk9wSO40ofamBFIlrOA
tm+jOGwOefiUqns2+b5xh0pCHILrY2i9qeZjzkTNWvYwWQpxNge2BGrnBL4m
XClGfKNm3x6pseWUqw787Q85WsEQylUSaQqkV3etTV33FVSYU6VVn6gdt5G9
d00ihp8WhDAurHdq8s8b.gmGnYLgLISqhuVmT6ZT0oCh8VORNsYjnmiQhzlQ
hbNFI6VLRNmC5oNKjWcjTpb69H41F58h9fasLGKSHEaOr.SwXmwpKnxps2zU
Ki1UKyOz0XLg0srvNaYaAeiVlZeW3d7NhoYLFpqlYjwA6tqqPizUn4NlZ3TP
cnEzYKi1FKSomPAstM8zlvT54HgfaaRxQOGIDntskdNccjb57JncAZ10i55L
BBZKB57H0pruNuNCpNb8w5u.vf5k34EyXE5cB0CVpcaszfcueeCkb4MzeXFt
AC0sAC0oSFpSmqGRsCLJ5Zb8uQJWTd9irBQ0SWaTnN1G3ZGIXn91jLysZ82V
ErGS11eSKnx9kIRTcYYg1QrVWcr+Vq3HRxJSpl9PbfCoVir9+xD0+9Cls1iR
ou5aW8cPshRPJ
-----------end_max5_patcher-----------
</code></pre>
</details>