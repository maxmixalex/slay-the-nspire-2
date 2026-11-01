"""Independent source-equation and complete interpolation-bound checks."""
import ctypes,math,sys,tempfile,unittest
from pathlib import Path
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import ancient_turbulence_ui23 as a


def reference_noise(point):
    # Literal vector form and all16 corner weights, independent of the native
    # helper's in-place W/Z/Y/X interpolation implementation.
    p=np.asarray(point,dtype=np.float32)
    dot=lambda x,y:np.float32(np.float32(np.float32(x[0]*y[0]+x[1]*y[1])+x[2]*y[2])+x[3]*y[3])
    shift=dot(p,np.array([-.1666667,-.1666667,-.1666667,-.5],np.float32))
    q=np.r_[p[:3]+shift,dot(p,np.full(4,.5,np.float32))].astype(np.float32)
    base=np.floor(q);delta=q-base;fade=delta*delta*delta*(np.float32(10)+delta*(np.float32(-15)+delta*np.float32(6)))
    matrix=np.array([[.143081,.001724,.280166,.262771],[.645401,-.047791,-.146698,.595016],[-.499665,-.095734,.425674,-.207367],[-.013596,-.848588,.423736,.17044]],np.float32)
    values=[]
    for index in range(16):
        corner=np.array([(index>>j)&1 for j in range(4)],np.float32)
        g=np.array([dot(base+corner,row) for row in matrix],np.float32);g-=np.floor(g)
        g=g*np.roll(g,-1)*np.float32(2365.952041);g=(g-np.floor(g))*np.float32(2)-np.float32(1)
        values.append(dot(delta-corner,g))
    # Keep the original vector-expression association for the finite-curl oracle.
    v=np.array(values,np.float32)
    v=v[:8]*(np.float32(1)-fade[3])+v[8:]*fade[3]
    v=v[:4]*(np.float32(1)-fade[2])+v[4:]*fade[2]
    v=v[:2]*(np.float32(1)-fade[1])+v[2:]*fade[1]
    return np.float32(v[0]*(np.float32(1)-fade[0])+v[1]*fade[0])


def reference_curl(point,strength):
    p=np.asarray(point,dtype=np.float32);epsilon=np.float32(.001)+np.float32(max(strength-1,0))*np.float32(70)
    offset=np.float32(1.7320508)*np.float32(2048.333333)
    def three(q):
        plus=q.copy();plus[3]+=offset;minus=q.copy();minus[3]-=offset
        return np.array([reference_noise(q),reference_noise(plus),reference_noise(minus)],np.float32)
    pairs=[]
    for axis in range(3):
        low=p.copy();high=p.copy();low[axis]-=epsilon;high[axis]+=epsilon;pairs.append((three(low),three(high)))
    (x0,x1),(y0,y1),(z0,z1)=pairs
    r=np.array([(y1[2]-y0[2])-(z1[1]-z0[1]),(z1[0]-z0[0])-(x1[2]-x0[2]),(x1[1]-x0[1])-(y1[0]-y0[0])],np.float32)
    return r/np.sqrt(np.float32(np.float32(r[0]*r[0]+r[1]*r[1])+r[2]*r[2]))

class SourceTurbulence(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.work=tempfile.TemporaryDirectory();cls.native=a.helper(cls.work.name)
    @classmethod
    def tearDownClass(cls):cls.work.cleanup()
    def test_original_float32_gradient_and_full_eighteen_sample_curl(self):
        random=np.random.default_rng(0x451);points=np.r_[random.uniform(-10,10,(30,4)),[[0,0,0,0],[1,-1,.5,3547.]],random.uniform(-2000,2000,(10,4))].astype('<f4')
        for p in points:
            actual=self.native.nspire_godot_noise4(p.ctypes.data_as(a.FLOAT_P))
            self.assertLessEqual(abs(actual-float(reference_noise(p))),2e-6)
        for strength in (1,20):
            for p in points[::6]:
                actual=np.zeros(3,dtype='<f4');self.native.nspire_godot_curl4(p.ctypes.data_as(a.FLOAT_P),strength,actual.ctypes.data_as(a.FLOAT_P))
                np.testing.assert_allclose(actual,reference_curl(p,strength),rtol=2e-5,atol=2e-5)
    def test_no_influence_retains_semiimplicit_source_gravity_and_radial_force(self):
        config=np.zeros(37,dtype='<f4');config[[0,1,4,5,6,7,8,14,15,22,23,28]]=[1,30,2,1,0,1,1,1,1,1,9,1]
        config[13]=70;curve=np.ones(256,dtype='<f4')
        track,constant=a.trajectory(self.native,config,curve,0,12345)
        n=np.arange(31,dtype=np.float64);expected=70/(30*30)*n*(n+1)/2
        np.testing.assert_allclose(track[:,1]-track[0,1],expected,rtol=1e-6,atol=1e-5)
        np.testing.assert_allclose(track[:,0],track[0,0],atol=1e-6)
        config[13]=0;config[35:]=28.849997
        radial,_=a.trajectory(self.native,config,curve,0,12345)
        sign=1 if radial[0,0]>0 else -1
        expected=radial[0,0]+sign*float(config[35])/(30*30)*n*(n+1)/2
        np.testing.assert_allclose(radial[:,0],expected,rtol=2e-6,atol=2e-5)
        np.testing.assert_equal(constant,[1,0])
    def test_bounded_trajectory_at_every_fractional_source_tick(self):
        random=np.random.default_rng(61023)
        for size in (2,31,421,901):
            source=np.cumsum(random.normal(0,.17,(size,2)),axis=0)+[120.123,50.456]
            raw,proof=a.compress_track(source);keys=np.frombuffer(raw,dtype=np.dtype([('tick','<u2'),('x','<i2'),('y','<i2')]))
            # Exercise display times between source ticks, using the actual
            # integer Q4-to-Q8 signed division performed by the calculator.
            for at in np.arange(0,size-1+1e-9,.125):
                lo=max(0,min(len(keys)-2,int(np.searchsorted(keys['tick'],at,side='right'))-1));left,right=keys[lo],keys[lo+1]
                f=(at-int(left['tick']))/(int(right['tick'])-int(left['tick']))
                q=np.array([left['x'],left['y']],np.int64)*16
                delta=np.array([int(right['x'])-int(left['x']),int(right['y'])-int(left['y'])])*16
                actual=(q+np.trunc(delta*f).astype(np.int64))/256
                tick=min(size-2,int(at));reference=source[tick]+(source[tick+1]-source[tick])*(at-tick)
                self.assertLessEqual(float(np.linalg.norm(actual-reference)),a.ERROR_PIXELS+1e-9)
            self.assertLessEqual(proof['maximum_lcd_error_bound'],.125)
        with self.assertRaises(ValueError):a.compress_track([[0,0],[3000,3000]])

if __name__=='__main__':unittest.main()
