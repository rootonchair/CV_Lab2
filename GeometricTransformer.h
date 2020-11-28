#pragma once
#include <iostream>
#include "opencv2/opencv.hpp"

using namespace cv;

/*
 Lớp base dùng để nội suy màu của 1 pixel
*/
class PixelInterpolate
{
public:
	/*
	Hàm tính giá trị màu của ảnh kết quả từ nội suy màu trong ảnh gốc
	Tham số
		- (tx,ty): tọa độ thực của ảnh gốc sau khi thực hiện phép biến đổi affine
		- pSrc: con trỏ ảnh gốc
		- returnedVal: con trỏ nhận giá trị nội suy được
		- srcWidthStep: widthstep của ảnh gốc
		- nChannels: số kênh màu của ảnh gốc
	*/
	virtual void Interpolate(
		float tx, float ty, uchar* pSrc, uchar* returnedVal, int srcWidthStep, int nChannels) = 0;
	
	PixelInterpolate() 
	{

	}

	virtual ~PixelInterpolate() 
	{

	}
};

/*
Lớp nội suy màu theo phương pháp song tuyến tính
*/
class BilinearInterpolate : public PixelInterpolate
{
public:
	void Interpolate(float tx, float ty, uchar* pSrc, uchar* returnedVal, int srcWidthStep, int nChannels)
	{
		float a = tx - floorf(tx);
		float b = ty - floorf(ty);

		for (int c = 0; c < nChannels; c++) {
			uchar topLeft = *(pSrc + srcWidthStep * (int)ty + nChannels * (int)tx + c);
			uchar topRight = (floorf(tx) == tx ? 0 : *(pSrc + srcWidthStep * (int)ty + nChannels * (int)(tx + 1) + c)); // Tránh truy cập vùng nhớ không được phép
			uchar bottomLeft = *(pSrc + srcWidthStep * (int)(ty + 1) + nChannels * (int)tx + c);
			uchar bottomRight = (floorf(ty) == ty ? 0 : *(pSrc + srcWidthStep * (int)(ty + 1) + nChannels * (int)(tx + 1) + c)); // Tránh truy cập vùng nhớ không được phép
			uchar topVal = (uchar)((1.0 - a)*topLeft + a * topRight);
			uchar bottomVal = (uchar)((1.0 - a)*bottomLeft + a * bottomRight);
			returnedVal[c] = (1 - b)*topVal + b * bottomVal;
		}
		
	}

	BilinearInterpolate() 
	{

	}

	~BilinearInterpolate() 
	{

	}
};

/*
Lớp nội suy màu theo phương pháp láng giềng gần nhất
*/
class NearestNeighborInterpolate : public PixelInterpolate
{
public:
	void Interpolate(float tx, float ty, uchar* pSrc, uchar* returnedVal, int srcWidthStep, int nChannels)
	{
		int nearestX = (int)(tx + 0.5f);
		int nearestY = (int)(ty + 0.5f);

		uchar* neighborPixel = pSrc + nearestY * srcWidthStep + nearestX * nChannels;

		for (int i = 0; i < nChannels; i++) {
			returnedVal[i] = neighborPixel[i];
		}
	}

	NearestNeighborInterpolate() 
	{

	}

	~NearestNeighborInterpolate() 
	{

	}
};

/*
Lớp biểu diễn pháp biến đổi affine
*/
class AffineTransform
{
	Mat _matrixTransform;//ma trận 3x3 biểu diễn phép biến đổi affine
public:
	void Translate(float dx, float dy);// xây dựng matrix transform cho phép tịnh tiến theo vector (dx,dy)
	void Rotate(float angle);//xây dựng matrix transform cho phép xoay 1 góc angle
	void Scale(float sx, float sy);//xây dựng matrix transform cho phép tỉ lệ theo hệ số 		
	void TransformPoint(float &x, float &y);//transform 1 điểm (x,y) theo matrix transform đã có
	
	AffineTransform();
	~AffineTransform();
};

/*
Lớp thực hiện phép biến đổi hình học trên ảnh
*/

class GeometricTransformer
{
public:
	/*
	Hàm biến đổi ảnh theo 1 phép biến đổi affine đã có
	Tham số
	 - beforeImage: ảnh gốc trước khi transform
	 - afterImage: ảnh sau khi thực hiện phép biến đổi affine
	 - transformer: phép biến đổi affine ngược
	 - interpolator: biến chỉ định phương pháp nội suy màu
	Trả về:
	 - 0: Nếu ảnh input ko tồn tại hay ko thực hiện được phép biến đổi
	 - 1: Nếu biến đổi thành công
	*/
	int Transform(
		const Mat &beforeImage, 
		Mat &afterImage, 
		AffineTransform* transformer, 
		PixelInterpolate* interpolator);

	/*
	Hàm xoay bảo toàn nội dung ảnh theo góc xoay cho trước
	Tham số
	- srcImage: ảnh input
	- dstImage: ảnh sau khi thực hiện phép xoay
	- angle: góc xoay (đơn vị: độ)
	- interpolator: biến chỉ định phương pháp nội suy màu
	Trả về:
	 - 0: Nếu ảnh input ko tồn tại hay ko thực hiện được phép biến đổi
	 - 1: Nếu biến đổi thành công
	*/
	int RotateKeepImage(
		const Mat &srcImage, Mat &dstImage, float angle, PixelInterpolate* interpolator);

	/*
	Hàm xoay không bảo toàn nội dung ảnh theo góc xoay cho trước
	Tham số
	- srcImage: ảnh input
	- dstImage: ảnh sau khi thực hiện phép xoay
	- angle: góc xoay (đơn vị: độ)
	- interpolator: biến chỉ định phương pháp nội suy màu
	Trả về:
	 - 0: Nếu ảnh input ko tồn tại hay ko thực hiện được phép biến đổi
	 - 1: Nếu biến đổi thành công
	*/
	int RotateUnkeepImage(
		const Mat &srcImage, Mat &dstImage, float angle, PixelInterpolate* interpolator);

	/*
	Hàm phóng to, thu nhỏ ảnh theo tỉ lệ cho trước
	Tham số
	- srcImage: ảnh input
	- dstImage: ảnh sau khi thực hiện phép xoay	
	- sx, sy: tỉ lệ phóng to, thu nhỏ ảnh	
	- interpolator: biến chỉ định phương pháp nội suy màu
	Trả về:
	 - 0: Nếu ảnh input ko tồn tại hay ko thực hiện được phép biến đổi
	 - 1: Nếu biến đổi thành công
	*/
	int Scale(
		const Mat &srcImage, 
		Mat &dstImage, 
		float sx, float sy, 
		PixelInterpolate* interpolator);
		
	
	/*
	Hàm thay đổi kích thước ảnh
	Tham số
	- srcImage: ảnh input
	- dstImage: ảnh sau khi thay đổi kích thước
	- newWidth, newHeight: kích thước mới
	- interpolator: biến chỉ định phương pháp nội suy màu
	Trả về:
	 - 0: Nếu ảnh input ko tồn tại hay ko thực hiện được phép biến đổi
	 - 1: Nếu biến đổi thành công
	*/
	int Resize(
		const Mat &srcImage, 
		Mat &dstImage, 
		int newWidth, int newHeight, 
		PixelInterpolate* interpolator);

	/*
	Hàm lấy đối xứng ảnh
	Tham số
	- srcImage: ảnh input
	- dstImage: ảnh sau khi lấy đối xứng
	- direction = 1 nếu lấy đối xứng theo trục ngang và direction = 0 nếu lấy đối xứng theo trục đứng
	- interpolator: biến chỉ định phương pháp nội suy màu
	Trả về:
	 - 0: Nếu ảnh input ko tồn tại hay ko thực hiện được phép biến đổi
	 - 1: Nếu biến đổi thành công
	*/
	int Flip(
		const Mat &srcImage, 
		Mat &dstImage, 
		bool direction,
		PixelInterpolate* interpolator);

	GeometricTransformer();
	~GeometricTransformer();
};

