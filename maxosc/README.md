# MaxOsc

Library to call python code from MaxMSP over OSC. 

Use in combination with the pyosc package in MaxMSP.

Further documentation will be available later.

### Minimal Example
##### Python Code:
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

##### Max Code:
!["Max code"]("./docs/misc/readme1.png")

<details><summary>Show code</summary>

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

TODO