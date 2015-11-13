#include <gdal_priv.h>
typedef unsigned short ushort;
typedef unsigned char uchar;

void CreateFile(const char *srcfile,const char *dstfile)
{
	GDALAllRegister();
	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8","NO");
	GDALDataset *pDataset=(GDALDataset *) GDALOpen( srcfile, GA_ReadOnly );
	int bandNum=pDataset->GetRasterCount();
	GDALRasterBand *pBand=pDataset->GetRasterBand(1);
	GDALDataType dataType=pBand->GetRasterDataType();

	GDALDriver *pDriver=GetGDALDriverManager()->GetDriverByName("GTiff");
	GDALDataset *dstDataset=pDriver->Create(dstfile,800,800,bandNum,dataType,NULL);
	GDALRasterBand *dstBand;
	//写入光栅数据
	ushort *buf= new ushort[800*800];
	for (int i=1;i<=bandNum;i++)
	{
		pBand=pDataset->GetRasterBand(i);
		pBand->RasterIO(GF_Read,3388,2204,800,800,buf,800,800,dataType,0,0);
		dstBand=dstDataset->GetRasterBand(i);
		dstBand->RasterIO(GF_Write,0,0,800,800,buf,800,800,dataType,0,0);
	}
	delete []buf;
	GDALClose(pDataset);
	GDALClose(dstDataset);
}

//直接最大最小值拉伸
void MinMaxStretch(ushort *pBuf,uchar * dstBuf,int bufWidth,int bufHeight,double minVal,double maxVal)
{
	ushort data;
	uchar result;
	for (int x=0;x<bufWidth;x++)
	{
		for (int y=0;y<bufHeight;y++)
		{
			data=pBuf[x*bufHeight+y];
			result=(data-minVal)/(maxVal-minVal)*255;
			dstBuf[x*bufHeight+y]=result;
		}
	}
}

//改进的最大最小值拉伸
void MinMaxStretchNew(ushort *pBuf,uchar *dstBuf,int bufWidth,int bufHeight,double minVal,double maxVal)
{
	ushort data;
	uchar result;
	for (int x=0;x<bufWidth;x++)
	{
		for (int y=0;y<bufHeight;y++)
		{
			data=pBuf[x*bufHeight+y];
			if (data>maxVal)
			{
				result=255;
			}
			else if (data<minVal)
			{
				result=0;
			}
			else
			{
				result=(data-minVal)/(maxVal-minVal)*255;
			}
			dstBuf[x*bufHeight+y]=result;
		}
	}
}

void HistogramAccumlateMinMax16S(ushort *pBuf,int width,int height,double *minVal,double *maxVal)
{
	double p[1024],p1[1024],num[1024];

	memset(p,0,sizeof(p));
	memset(p1,0,sizeof(p1));
	memset(num,0,sizeof(num));

	long wMulh = height * width;

	//statistics
	for(int x=0;x<width;x++)
	{
		for(int y=0;y<height;y++){
			ushort v=pBuf[x*height+y];
			num[v]++;
		}
	}

	//calculate probability
	for(int i=0;i<1024;i++)
	{
		p[i]=num[i]/wMulh;
	}

	int min=0,max=0;
	double minProb=0.0,maxProb=0.0;
	while(min<1024&&minProb<0.02)
	{
		minProb+=p[min];
		min++;
	}
	do 
	{
		maxProb+=p[max];
		max++;
	} while (max<1024&&maxProb<0.98);

	*minVal=min;
	*maxVal=max;
}

void Create8BitImage(const char *srcfile,const char *dstfile)
{
	GDALAllRegister();
	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8","NO");
	GDALDataset *pDataset=(GDALDataset *) GDALOpen( srcfile, GA_ReadOnly );
	int bandNum=pDataset->GetRasterCount();

	GDALDriver *pDriver=GetGDALDriverManager()->GetDriverByName("GTiff");
	GDALDataset *dstDataset=pDriver->Create(dstfile,800,800,3,GDT_Byte,NULL);
	GDALRasterBand *pBand;
	GDALRasterBand *dstBand;
	//写入光栅数据
	ushort *sbuf= new ushort[800*800];
	uchar *cbuf=new uchar[800*800];
	for (int i=bandNum,j=1;i>=2;i--,j++)
	{
		pBand=pDataset->GetRasterBand(i);
		pBand->RasterIO(GF_Read,0,0,800,800,sbuf,800,800,GDT_UInt16,0,0);
		/*int bGotMin, bGotMax;
		double adfMinMax[2];
		adfMinMax[0] = pBand->GetMinimum( &bGotMin );
		adfMinMax[1] = pBand->GetMaximum( &bGotMax );
		if( ! (bGotMin && bGotMax) )
			GDALComputeRasterMinMax((GDALRasterBandH)pBand, TRUE, adfMinMax);
		MinMaxStretch(sbuf,cbuf,800,800,adfMinMax[0],adfMinMax[1]);*/
		double min,max;
		HistogramAccumlateMinMax16S(sbuf,800,800,&min,&max);
		MinMaxStretchNew(sbuf,cbuf,800,800,min,max);
		dstBand=dstDataset->GetRasterBand(j);
		dstBand->RasterIO(GF_Write,0,0,800,800,cbuf,800,800,GDT_Byte,0,0);
	}
	delete []cbuf;
	delete []sbuf;
	GDALClose(pDataset);
	GDALClose(dstDataset);
}

int main(int argc,char** argv)
{
	/*CreateFile("J:\\tiff-mux\\ZY3_MUX_E112.2_N29.9_20131222_L1A0001084660.tiff"
		,"C:\\Users\\Dell\\Desktop\\assets\\mux16bit.tiff");*/
	Create8BitImage("C:\\Users\\Dell\\Desktop\\assets\\mux16bit.tiff",
		"C:\\Users\\Dell\\Desktop\\assets\\mux16bit81.tiff");
}