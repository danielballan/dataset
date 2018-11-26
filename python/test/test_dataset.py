import unittest

from dataset import *
import numpy as np

class TestDataset(unittest.TestCase):
    def setUp(self):
        lx = 2
        ly = 3
        lz = 4
        self.dataset = Dataset()
        dims = Dimensions()
        dims.add(Dim.X, lx)
        dims.add(Dim.Y, ly)
        dims.add(Dim.Z, lz)

        self.reference_x = np.arange(lx)
        self.reference_y = np.arange(ly)
        self.reference_z = np.arange(lz)
        self.reference_data1 = np.arange(24).reshape(4,3,2)
        self.reference_data2 = np.ones(24).reshape(4,3,2)

        self.dataset.insert(Data.Value, "data1", dims, np.arange(24))
        self.dataset.insert(Data.Value, "data2", dims, np.ones(24))

        dimsX = Dimensions()
        dimsX.add(Dim.X, lx)
        self.dataset.insert(Coord.X, dimsX, self.reference_x)
        dimsY = Dimensions()
        dimsY.add(Dim.Y, ly)
        self.dataset.insert(Coord.Y, dimsY, self.reference_y)
        dimsZ = Dimensions()
        dimsZ.add(Dim.Z, lz)
        self.dataset.insert(Coord.Z, dimsZ, self.reference_z)

    def test_size(self):
        # X, Y, Z, 2xData::Value
        self.assertEqual(self.dataset.size(), 5)

    def test_dimensions(self):
        self.assertEqual(self.dataset.dimensions().size(Dim.X), 2)
        self.assertEqual(self.dataset.dimensions().size(Dim.Y), 3)
        self.assertEqual(self.dataset.dimensions().size(Dim.Z), 4)

    def test_data(self):
        np.testing.assert_array_equal(self.dataset[Coord.X].numpy, self.reference_x)
        np.testing.assert_array_equal(self.dataset[Coord.Y].numpy, self.reference_y)
        np.testing.assert_array_equal(self.dataset[Coord.Z].numpy, self.reference_z)
        np.testing.assert_array_equal(self.dataset[Data.Value, "data1"].numpy, self.reference_data1)
        np.testing.assert_array_equal(self.dataset[Data.Value, "data2"].numpy, self.reference_data2)

    def test_view_subdata(self):
        view = self.dataset["data1"]
        # TODO Need consistent dimensions() implementation for Dataset and its views.
        #self.assertEqual(view.dimensions().size(Dim.X), 2)
        #self.assertEqual(view.dimensions().size(Dim.Y), 3)
        #self.assertEqual(view.dimensions().size(Dim.Z), 4)
        self.assertEqual(view.size(), 4)

    def test_slice_dataset(self):
        for x in range(2):
            view = self.dataset[Dim.X, x]
            self.assertRaisesRegex(RuntimeError, 'Dataset does not contain such a variable.', view.__getitem__, Coord.X)
            np.testing.assert_array_equal(view[Coord.Y].numpy, self.reference_y)
            np.testing.assert_array_equal(view[Coord.Z].numpy, self.reference_z)
            np.testing.assert_array_equal(view[Data.Value, "data1"].numpy, self.reference_data1[:,:,x])
            np.testing.assert_array_equal(view[Data.Value, "data2"].numpy, self.reference_data2[:,:,x])
        for y in range(3):
            view = self.dataset[Dim.Y, y]
            np.testing.assert_array_equal(view[Coord.X].numpy, self.reference_x)
            self.assertRaisesRegex(RuntimeError, 'Dataset does not contain such a variable.', view.__getitem__, Coord.Y)
            np.testing.assert_array_equal(view[Coord.Z].numpy, self.reference_z)
            np.testing.assert_array_equal(view[Data.Value, "data1"].numpy, self.reference_data1[:,y,:])
            np.testing.assert_array_equal(view[Data.Value, "data2"].numpy, self.reference_data2[:,y,:])
        for z in range(4):
            view = self.dataset[Dim.Z, z]
            np.testing.assert_array_equal(view[Coord.X].numpy, self.reference_x)
            np.testing.assert_array_equal(view[Coord.Y].numpy, self.reference_y)
            self.assertRaisesRegex(RuntimeError, 'Dataset does not contain such a variable.', view.__getitem__, Coord.Z)
            np.testing.assert_array_equal(view[Data.Value, "data1"].numpy, self.reference_data1[z,:,:])
            np.testing.assert_array_equal(view[Data.Value, "data2"].numpy, self.reference_data2[z,:,:])

if __name__ == '__main__':
    unittest.main()
