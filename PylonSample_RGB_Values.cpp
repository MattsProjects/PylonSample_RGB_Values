// PylonSample_RGB_Values.cpp
/*
Note: Before getting started, Basler recommends reading the Programmer's Guide topic
in the pylon C++ API documentation that gets installed with pylon.
If you are upgrading to a higher major version of pylon, Basler also
strongly recommends reading the Migration topic in the pylon C++ API documentation.

This sample illustrates how to access RGB values of a color image.
*/

// Include files to use the PYLON API.
#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif

// Namespace for using pylon objects.
using namespace Pylon;

// Namespace for using cout.
using namespace std;

// Number of images to be grabbed.
static const uint32_t c_countOfImagesToGrab = 1;

int main(int argc, char* argv[])
{
	// The exit code of the sample application.
	int exitCode = 0;

	// Automagically call PylonInitialize and PylonTerminate to ensure the pylon runtime system
	// is initialized during the lifetime of this object.
	Pylon::PylonAutoInitTerm autoInitTerm;

	try
	{
		CDeviceInfo info;
		info.SetSerialNumber("21824812");

		// Create an instant camera object with the camera device found first.
		CInstantCamera camera(CTlFactory::GetInstance().CreateFirstDevice(info));

		// Print the model name of the camera.
		cout << "Using device " << camera.GetDeviceInfo().GetModelName() << endl;

		// open the camera so we can configure the physical device itself.
		camera.Open();

		// set original pixel format from camera
		GenApi::CEnumerationPtr(camera.GetNodeMap().GetNode("PixelFormat"))->FromString("RGB8");

		// This smart pointer will receive the grab result data.
		CGrabResultPtr ptrGrabResult;

		// We may need a Pylon Image Format Converter to convert between Bayer, RGB, etc. on the Host PC.
		Pylon::CImageFormatConverter myImageFormatConverter;

		// Method #1: We can hold an RGB image in one Pylon Image
		Pylon::CPylonImage myRGBImage;

		// Method #2: Or we can separate an RGB-Planar Image into three images
		CPylonImage myRGBPlanarImage;
		CPylonImage myRedImage;
		CPylonImage myGreenImage;
		CPylonImage myBlueImage;

		// Start the grabbing of c_countOfImagesToGrab images.
		camera.StartGrabbing(c_countOfImagesToGrab);

		while (camera.IsGrabbing())
		{
			// Wait for an image and then retrieve it. A timeout of 5000 ms is used.
			// Camera.StopGrabbing() is called automatically by RetrieveResult() when c_countOfImagesToGrab have been grabbed.
			camera.RetrieveResult(5000, ptrGrabResult, TimeoutHandling_ThrowException);

			// Image grabbed successfully?
			if (ptrGrabResult->GrabSucceeded())
			{
				// Find the format of the image within the Grab Result
				PixelType pixelType = ptrGrabResult->GetPixelType();
				cout << "Image Pixel format: " << Pylon::CPixelTypeMapper::GetNameByPixelType(pixelType) << endl;

				// if the image within the grab result is not color, notify the user.
				if (IsColorImage(pixelType) == false)
				{
					cout << "Image is not color. Please set camera's Pixel Format to either Bayer, RGB, or YUV/YCbCr." << endl;
				}
				else
				{
					// If the image is color, but not RGB format (eg Bayer or YUV/YCbCr), we can convert it to RGB on the host.
					if (IsRGB(pixelType) == false)
					{
						// set the output format of the Image Format Converter
						myImageFormatConverter.OutputPixelFormat = PixelType_RGB8packed;

						// convert the image in the Grab Result to RGB and place it into myRGBImage.
						myImageFormatConverter.Convert(myRGBImage, ptrGrabResult);
					}
					else
					{
						// The image in the Grab Result is already RGB, so attach it directly to myRGBImage.
						myRGBImage.AttachGrabResultBuffer(ptrGrabResult);
					}

					// Now access the RGB values of the pixels in the image.

					// Method #1: access the RGB pixel values by using a pointer to myRGBImage's buffer
					{
						uint8_t *pBuffer = (uint8_t *)myRGBImage.GetBuffer();

						cout << endl;
						cout << "Accessing RGB values of image..." << endl;
						cout << "Red Value of first pixel   : " << (uint32_t)pBuffer[0] << endl;
						cout << "Green Value of first pixel : " << (uint32_t)pBuffer[1] << endl;
						cout << "Blue Value of first pixel  : " << (uint32_t)pBuffer[2] << endl;
					}

					// Method #2: Convert the image to RGB planar format and split into three images
					{
						// reset the output format of the converter
						myImageFormatConverter.OutputPixelFormat = PixelType_RGB8planar;

						// convert to RGB planar
						myImageFormatConverter.Convert(myRGBPlanarImage, myRGBImage);

						// separate the planar images into three images for R,G,B
						myRedImage = myRGBPlanarImage.GetPlane(0);
						myGreenImage = myRGBPlanarImage.GetPlane(1);
						myBlueImage = myRGBPlanarImage.GetPlane(2);

						// Access the pixel values of each image.
						uint8_t *pRed = (uint8_t *)myRedImage.GetBuffer();
						uint8_t *pGreen = (uint8_t *)myGreenImage.GetBuffer();
						uint8_t *pBlue = (uint8_t *)myBlueImage.GetBuffer();

						cout << endl;
						cout << "Accessing RGB values of RGB image converted to planar format..." << endl;
						cout << "Value of first pixel in Red Plane   : " << (uint32_t)pRed[0] << endl;
						cout << "Value of first pixel in Green Plane : " << (uint32_t)pGreen[0] << endl;
						cout << "Value of first pixel in Blue Plane  : " << (uint32_t)pBlue[0] << endl;

						// Save the image(s)
						CImagePersistence::Save(ImageFileFormat_Bmp, "RGB.bmp", myRGBImage);
						CImagePersistence::Save(ImageFileFormat_Bmp, "red_plane.bmp", myRedImage);
						CImagePersistence::Save(ImageFileFormat_Bmp, "green_plane.bmp", myGreenImage);
						CImagePersistence::Save(ImageFileFormat_Bmp, "blue_plane.bmp", myBlueImage);

						// Display the image(s)
						Pylon::DisplayImage(0, myRGBImage);
						Pylon::DisplayImage(1, myRedImage); // red channel
						Pylon::DisplayImage(2, myGreenImage); // green channel
						Pylon::DisplayImage(3, myBlueImage); // blue channel
					}
				}
			}
			else
			{
				cout << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << endl;
			}
		}
	}
	catch (GenICam::GenericException &e)
	{
		// Error handling.
		cerr << "An exception occurred." << endl
			<< e.GetDescription() << endl;
		exitCode = 1;
	}

	// Comment the following two lines to disable waiting on exit.
	cerr << endl << "Press Enter to exit." << endl;
	while (cin.get() != '\n');

	return exitCode;
}
