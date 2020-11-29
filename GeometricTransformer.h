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
	void Translate(float dx, float dy)// xây dựng matrix transform cho phép tịnh tiến theo vector (dx,dy)
	{
		float translateMatrixData[9] =
		{
			1.0f, 0.0f,   dx,
			0.0f, 1.0f,   dy,
			0.0f, 0.0f, 1.0f
		};

		Mat translateMatrix(3, 3, CV_32FC1, translateMatrixData);
		this->_matrixTransform = translateMatrix * this->_matrixTransform;
	}

	void Rotate(float angle)//xây dựng matrix transform cho phép xoay 1 góc angle ngược chiều kim đồng hồ
	{
		float angleInRadian = angle * CV_PI / 180.0f;
		float rotateMatrixData[9] =
		{
			cos(angleInRadian), -sin(angleInRadian), 0.0f,
			sin(angleInRadian),  cos(angleInRadian), 0.0f,
						  0.0f,                0.0f, 1.0f
		};

		Mat rotateMatrix(3, 3, CV_32FC1, rotateMatrixData);
		this->_matrixTransform = rotateMatrix * this->_matrixTransform;
	}

	void Scale(float sx, float sy)//xây dựng matrix transform cho phép tỉ lệ theo hệ số
	{
		float scaleMatrixData[9] =
		{
			  sx, 0.0f, 0.0f,
			0.0f,   sy, 0.0f,
			0.0f, 0.0f, 1.0f
		};

		Mat scaleMatrix(3, 3, CV_32FC1, scaleMatrixData);
		this->_matrixTransform = scaleMatrix * this->_matrixTransform;
	}

	void TransformPoint(float &x, float &y)//transform 1 điểm (x,y) theo matrix transform đã có
	{
		float pointData[3] = { x, y, 1.0 };

		Mat point = Mat(3, 1, CV_32FC1, pointData);
		Mat result = this->_matrixTransform * point;

		x = result.ptr<float>(0)[0];
		y = result.ptr<float>(1)[0];
	}

	AffineTransform()
	{
		this->_matrixTransform = Mat::eye(3, 3, CV_32FC1);
	}
	~AffineTransform()
	{

	}
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
		PixelInterpolate* interpolator)
	{

		uchar* resultPixel = new uchar[beforeImage.channels()];
		for (int row = 0; row < afterImage.rows; row++)
		{
			for (int col = 0; col < afterImage.rows; col++)
			{
				float orgX = col;
				float orgY = row;

				transformer->TransformPoint(orgX, orgY);
				if (orgX > beforeImage.cols - 1 || orgX < 0)
					continue;

				if (orgY > beforeImage.rows - 1 || orgY < 0)
					continue;

				interpolator->Interpolate(orgX, orgY, beforeImage.data, resultPixel, beforeImage.step[0], beforeImage.step[1]);
				uchar* currentPixel = afterImage.data + afterImage.step[0] * row + afterImage.step[1] * col;
				for (int c = 0; c < beforeImage.channels(); c++) {
					currentPixel[c] = resultPixel[c];
				}
			}
		}

		delete[] resultPixel;
		return 1;
	}


	void transformAndFindMaxMin(AffineTransform& transformer, float x, float y, float &xmax, float &xmin, float &ymax, float &ymin) 
	{
		transformer.TransformPoint(x, y);
		if (x < xmin)
			xmin = x;
		else if (x > xmax)
			xmax = x;

		if (y < ymin)
			ymin = y;
		else if (y > ymax)
			ymax = y;
	}

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
		const Mat &srcImage, Mat &dstImage, float angle, PixelInterpolate* interpolator)
	{
		/*
			Để tìm width và height của ảnh kết quả, thuật toán:
				- Xây dựng một affine xuôi và đưa tọa độ bốn góc của ảnh gốc để có kết quả
				- Tìm x và y lớn nhất và nhỏ nhất để biết width và height cần có
		*/
		AffineTransform forwardTransformer;
		forwardTransformer.Translate(-srcImage.cols / 2.0f, -srcImage.rows / 2.0f);
		forwardTransformer.Rotate(-angle);

		float maxX, minX, maxY, minY;
		float x, y;
		x = 0, y = 0; //top left
		forwardTransformer.TransformPoint(x, y);
		maxX = x, minX = x;
		maxY = y, minY = y;

		transformAndFindMaxMin(forwardTransformer, srcImage.cols - 1, 0, maxX, minX, maxY, minY); // top right
		transformAndFindMaxMin(forwardTransformer, 0, srcImage.rows - 1, maxX, minX, maxY, minY); // bottom left
		transformAndFindMaxMin(forwardTransformer, srcImage.cols - 1, srcImage.rows - 1, maxX, minX, maxY, minY); // bottom right

		int dstWidth = (int)(maxX - minX + 0.5f);
		int dstHeight = (int)(maxY - minY + 0.5f);

		dstImage = Mat(dstHeight, dstWidth, srcImage.type(), Scalar(0));

		/*
			Phép affine xuôi sẽ có trình tự là:
				- dời ảnh về -width/2 và -height/2 của ảnh gốc
				- xoay ảnh theo hướng -angle do trái ngược về hệ trục tọa độ
				- dời ảnh về  width/2 và height/2 của ảnh kết quả (để không bị mất nội dung ảnh)
		*/
		AffineTransform backwardTransform;
		backwardTransform.Translate(-dstWidth / 2.0f, -dstHeight / 2.0f);
		backwardTransform.Rotate(angle);
		backwardTransform.Translate(srcImage.cols / 2.0f, srcImage.rows / 2.0f);

		return this->Transform(srcImage, dstImage, &backwardTransform, interpolator);

	}

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
		const Mat &srcImage, Mat &dstImage, float angle, PixelInterpolate* interpolator)
	{
		dstImage = Mat(srcImage.rows, srcImage.cols, srcImage.type(), Scalar(0));
		AffineTransform transformer;
		/*
		Để xoay ảnh cần:
			- trừ w/2 và h/2 để đưa ảnh về gốc tọa độ
			- xoay ảnh theo góc -angle (vì hệ trục bị ngược xuống)
			- cộng w/2 và h/2 để đưa ảnh về vị trí ban đầu

		Vậy affine ngược sẽ là:
			- cộng w/2 và h/2 để đưa ảnh về gốc tọa độ
			- xoay ảnh theo góc -angle (vì hệ trục bị ngược xuống)
			- cộng w/2 và h/2 để đưa ảnh về vị trí ban đầu
			
		*/
		transformer.Translate(-srcImage.cols / 2.0f, -srcImage.rows / 2.0f);
		transformer.Rotate(angle);
		transformer.Translate(srcImage.cols / 2.0f, srcImage.rows / 2.0f);
		return this->Transform(srcImage, dstImage, &transformer, interpolator);
	}

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
		PixelInterpolate* interpolator)
	{
		if (sx < FLT_EPSILON || sy < FLT_EPSILON)
			return 0;
		int dstRows = (int)(srcImage.rows * sy + 0.5f);
		int dstCols = (int)(srcImage.cols * sx + 0.5f);
		dstImage = Mat(dstRows, dstCols, srcImage.type());

		uchar* resultPixel = new uchar[srcImage.channels()];
		for (int row = 0; row < dstRows; row++)
		{
			for (int col = 0; col < dstCols; col++)
			{
				float orgX = col / sx;
				float orgY = row / sy;
				if (orgX > srcImage.cols - 1)
					orgX = srcImage.cols - 1;
				if (orgY > srcImage.rows - 1)
					orgY = srcImage.rows - 1;

				interpolator->Interpolate(orgX, orgY, srcImage.data, resultPixel, srcImage.step[0], srcImage.step[1]);
				uchar* currentPixel = dstImage.data + dstImage.step[0] * row + dstImage.step[1] * col;
				for (int c = 0; c < srcImage.channels(); c++) {
					currentPixel[c] = resultPixel[c];
				}
			}
		}

		delete[] resultPixel;
		return 1;
	}
		
	
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
		PixelInterpolate* interpolator)
	{
		if (newWidth <= 0 || newHeight <= 0)
			return 0;
		dstImage = Mat(newHeight, newWidth, srcImage.type());

		uchar* resultPixel = new uchar[srcImage.channels()];
		for (int row = 0; row < newHeight; row++)
		{
			for (int col = 0; col < newWidth; col++)
			{
				float orgX = 1.0f * col * (srcImage.cols - 1) / (newWidth - 1);
				float orgY = 1.0f * row * (srcImage.rows - 1) / (newHeight - 1);
				if (orgX > srcImage.cols - 1)
					orgX = srcImage.cols - 1;
				if (orgY > srcImage.rows - 1)
					orgY = srcImage.rows - 1;

				interpolator->Interpolate(orgX, orgY, srcImage.data, resultPixel, srcImage.step[0], srcImage.step[1]);
				uchar* currentPixel = dstImage.data + dstImage.step[0] * row + dstImage.step[1] * col;
				for (int c = 0; c < srcImage.channels(); c++) {
					currentPixel[c] = resultPixel[c];
				}
			}
		}

		delete[] resultPixel;
		return 1;
	}

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
		PixelInterpolate* interpolator)
	{
		if (direction > 1)
			return 0;

		dstImage = Mat(srcImage.rows, srcImage.cols, srcImage.type());

		for (int row = 0; row < dstImage.rows; row++) {
			for (int col = 0; col < dstImage.cols; col++) {
				int srcX, srcY;
				if (direction == 0) {
					srcX = dstImage.cols - 1 - col;
					srcY = row;
				}
				else if (direction == 1) {
					srcX = col;
					srcY = dstImage.rows - 1 - row;
				}

				uchar* srcPixel = srcImage.data + srcY * srcImage.step[0] + srcX * srcImage.step[1];
				uchar* dstPixel = dstImage.data + row * dstImage.step[0] + col * dstImage.step[1];

				for (int c = 0; c < dstImage.channels(); c++) {
					dstPixel[c] = srcPixel[c];
				}
			}
		}
		return 1;
	}

	GeometricTransformer() 
	{

	}

	~GeometricTransformer() 
	{

	}
};

