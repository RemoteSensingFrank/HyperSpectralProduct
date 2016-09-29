#include "StdAfx.h"
#include "PosProc.h"
#include "CoordinateTrans.h"
#include "matrixOperation.h"

//����POS���ݽ���EOԪ��
long GeoPOSProcess::GeoPOSProc_ExtractEO(const char* pPOSFile,const char* pEOFile,int nLines,int nBeginLine,THREEDPOINT THETA,float* fSetupAngle)
{
	long   lError = 0;
	double  dB = 0, dL = 0, dH = 0;
	int nQuarFlag = 0;
	FILE *fEO = NULL;
	float tempangle1 = float((fSetupAngle[0])*PI / 180.0);
	float tempangle2 = float((fSetupAngle[1])*PI / 180.0);

	//���ýǺͰ���ʸ��
	THREEDPOINT ANGLETHETA, POSISTTHETA;
	THREEDPOINT XYZPnt;
	CoordinateTransBasic m_coordinate;

	ANGLETHETA.dX = fSetupAngle[0]; ANGLETHETA.dY = fSetupAngle[1]; ANGLETHETA.dZ = fSetupAngle[2];
	POSISTTHETA.dX = THETA.dX;      POSISTTHETA.dY = THETA.dY;      POSISTTHETA.dZ = THETA.dZ;

	if ((lError = GeoPOSProc_ReadPartPOS(pPOSFile, nLines, nBeginLine)) != 0)
		goto ErrEnd;

	dB = m_Center_Geo_dB;
	dL= m_Center_Geo_dL;
	dH= m_Center_Geo_dH;

	//��ͼ����ϵ����������ϵ
	double EMMatrix[9];
	EMMatrix[0] = -sin(dL);
	EMMatrix[1] = cos(dL);
	EMMatrix[2] = 0;

	EMMatrix[3] = -sin(dB)*cos(dL);
	EMMatrix[4] = -sin(dB)*sin(dL);
	EMMatrix[5] = cos(dB);

	EMMatrix[6] = cos(dB)*cos(dL);
	EMMatrix[7] = cos(dB)*sin(dL);
	EMMatrix[8] = sin(dB);

	//�﷽����ϵ�Բ���������Ϊԭ��, ����Ϊ��(X)��(Y)��(Z)�ľֲ���ƽ��(��ظ�Ϊ0)����ϵ
	m_coordinate.BLHToXYZ(dB, dL, 0, XYZPnt);
	nQuarFlag = GeoPOSProc_EOQuadrant(this->m_Geo_Pos.size(), EMMatrix, XYZPnt);  //��������ʧ��

	if (nQuarFlag == 0)
	{
		lError = -1; //����EO�ļ�ʧ��
		goto ErrEnd;
	}

	//���·����Ϊ����EO�������
	if (pEOFile != NULL)
	{
		fopen_s(&fEO,pEOFile, "w");
		if (fEO == NULL)
		{
			lError = -1; //����EO�ļ�ʧ��
			goto ErrEnd;
		}

		fprintf(fEO, "image_Line_Numbers:  %d\n", this->m_Geo_Pos.size());
		fprintf(fEO, "EO_samples_of_lines: %d\n", 1);
		fprintf(fEO, "Average_Latitude: %.8f\n", dB);
		fprintf(fEO, "Average_Longitude: %.8f\n", dL);
		fprintf(fEO, "Average_Altitude: %.6f\n", dH);
		fprintf(fEO, "%20s%20s%20s%15s%15s%15s%10s%10s%10s%10s%10s%10s%10s%10s%10s",
			"Xs(m)", "Ys(m)", "Zs(m)", "phi(deg)", "omega(deg)", "kappa(deg)",
			"M[1]", "M[2]", "M[3]", "M[4]", "M[5]", "M[6]", "M[7]", "M[8]", "M[9]");

		ANGLETHETA.dX = ANGLETHETA.dX*PI / 180.0;
		ANGLETHETA.dY = ANGLETHETA.dY*PI / 180.0;
		ANGLETHETA.dZ = ANGLETHETA.dZ*PI / 180.0;
		for (int i = 0; i<(int)this->m_Geo_Pos.size(); i++)
			GeoPOSProc_EOMatrixTurn(i, XYZPnt, nQuarFlag, EMMatrix, ANGLETHETA, POSISTTHETA, fEO);

	}
	else
	{
		ANGLETHETA.dX = ANGLETHETA.dX*PI / 180.0;
		ANGLETHETA.dY = ANGLETHETA.dY*PI / 180.0;
		ANGLETHETA.dZ = ANGLETHETA.dZ*PI / 180.0;
		for (int i = 0; i<(int)this->m_Geo_Pos.size(); i++)
			GeoPOSProc_EOMatrixTurn(i, XYZPnt, nQuarFlag, EMMatrix, ANGLETHETA, POSISTTHETA, fEO);
	}
ErrEnd:
	if (fEO)
	{
		fclose(fEO);
		fEO = NULL;
	}

	return lError;
}

//����POS���ݼ��㺽����������
long GeoPOSProcess::GeoPOSProc_EOQuadrant(int nLines, double EMMatrix[], THREEDPOINT &XYZPoint)
{
	long nFlag = 0;
	CoordinateTransBasic m_coordinate;
	THREEDPOINT Pntstart, PntEnd;
	POS posBegin = this->m_Geo_Pos[0];
	POS posEnd = this->m_Geo_Pos[nLines - 1];
	m_coordinate.BLHToXYZ(posBegin.m_latitude, posBegin.m_longitude, posBegin.m_height, Pntstart);
	double dXstart = (Pntstart.dX - XYZPoint.dX)*EMMatrix[0] + (Pntstart.dY - XYZPoint.dY)*EMMatrix[1] + (Pntstart.dZ - XYZPoint.dZ)*EMMatrix[2];
	double dYstart = (Pntstart.dX - XYZPoint.dX)*EMMatrix[3] + (Pntstart.dY - XYZPoint.dY)*EMMatrix[4] + (Pntstart.dZ - XYZPoint.dZ)*EMMatrix[5];

	m_coordinate.BLHToXYZ(posEnd.m_latitude, posEnd.m_longitude, posEnd.m_height, PntEnd);			//WGS84
	double dXend = (PntEnd.dX - XYZPoint.dX)*EMMatrix[0] + (PntEnd.dY - XYZPoint.dY)*EMMatrix[1] + (PntEnd.dZ - XYZPoint.dZ)*EMMatrix[2];
	double dYend = (PntEnd.dX - XYZPoint.dX)*EMMatrix[3] + (PntEnd.dY - XYZPoint.dY)*EMMatrix[4] + (PntEnd.dZ - XYZPoint.dZ)*EMMatrix[5];

	double dx, dy;
	dx = dXend - dXstart;
	dy = dYend - dYstart;

	if (dx>0 && dy>0)		//����--1����
		nFlag = 1;
	else if (dx<0 && dy>0)    //����--2����
		nFlag = 2;
	else if (dx<0 && dy<0)    //����--3����
		nFlag = 3;
	else if (dx>0 && dy<0)    //����--4����
		nFlag = 4;

	return nFlag;
}

long GeoPOSProcess::GeoPOSProc_EOQuadrant(POS curPOS, double EMMatrix[], THREEDPOINT &XYZPoint)
{
	long nFlag = 0;
	double temps = curPOS.m_yaw*180.0 / PI;
	if (abs(int(temps)) % 360<10)					//North-East--1 quandrant
		nFlag = 1;
	else if (abs(int(temps - 90)) % 360<10)			//North_West--2 quandrant
		nFlag = 2;
	else if (abs(int(temps - 180)) % 360<10)		//South-West--3 quandrant
		nFlag = 3;
	else if (abs(int(temps - 270)) % 360<10)		//South-East--4 quandrant
		nFlag = 4;

	return nFlag;
}

//����POS��ȡEOԪ�غ���ת����������ļ�����������
long GeoPOSProcess::GeoPOSProc_EOMatrixTurn(int nCurLine, THREEDPOINT &XYZPoint, int nQuadNum, double EMMatrix[], THREEDPOINT ANGLETHETA, THREEDPOINT POSISTTHETA, FILE *fEO)
{
	//rotate matrix 
	double EGMatrix[9];	//	
	double GIMatrix[9];	//
	double CIMatrix[9];	//
	double ICMatrix[9];	//
	POS pdfPOS = this->m_Geo_Pos[nCurLine];
	double dL, dB, dH;
	double roll, pitch, yaw;
	dB =/*pfPOS[6*curpos+0]*/pdfPOS.m_latitude;
	dL =/*pfPOS[6*curpos+1]*/pdfPOS.m_longitude;
	dH =/*pfPOS[6*curpos+2]*/pdfPOS.m_height;

	roll =/*pfPOS[6*curpos+3]*/pdfPOS.m_roll;
	pitch =/*pfPOS[6*curpos+4]*/pdfPOS.m_pitch;
	yaw =/*pfPOS[6*curpos+5]*/pdfPOS.m_yaw;

	//WGS84 trans to local coordinate system
	EGMatrix[0] = -sin(dB)*cos(dL); EGMatrix[1] = -sin(dL); EGMatrix[2] = -cos(dB)*cos(dL);
	EGMatrix[3] = -sin(dB)*sin(dL); EGMatrix[4] = cos(dL) ; EGMatrix[5] = -cos(dB)*sin(dL);
	EGMatrix[6] = cos(dB)		  ;	EGMatrix[7] = 0		  ; EGMatrix[8] = -sin(dB)		;

	//Local coordinate system to IMU coordinate system
	GIMatrix[0] = cos(pitch)*cos(yaw);
	GIMatrix[1] = sin(roll)*sin(pitch)*cos(yaw) - cos(roll)*sin(yaw);
	GIMatrix[2] = cos(roll)*sin(pitch)*cos(yaw) + sin(roll)*sin(yaw);
	GIMatrix[3] = cos(pitch)*sin(yaw);
	GIMatrix[4] = sin(roll)*sin(pitch)*sin(yaw) + cos(roll)*cos(yaw);
	GIMatrix[5] = cos(roll)*sin(pitch)*sin(yaw) - sin(roll)*cos(yaw);
	GIMatrix[6] = -sin(pitch);
	GIMatrix[7] = sin(roll)*cos(pitch);
	GIMatrix[8] = cos(roll)*cos(pitch);

	//IMU to sensor coordinate system trans
	CIMatrix[0] = cos(ANGLETHETA.dY)*cos(ANGLETHETA.dZ);
	CIMatrix[1] = cos(ANGLETHETA.dY)*sin(ANGLETHETA.dZ);
	CIMatrix[2] = -sin(ANGLETHETA.dY);
	CIMatrix[3] = sin(ANGLETHETA.dX)*sin(ANGLETHETA.dY)*cos(ANGLETHETA.dZ) - cos(ANGLETHETA.dX)*sin(ANGLETHETA.dZ);
	CIMatrix[4] = sin(ANGLETHETA.dX)*sin(ANGLETHETA.dY)*sin(ANGLETHETA.dZ) + cos(ANGLETHETA.dX)*cos(ANGLETHETA.dZ);
	CIMatrix[5] = sin(ANGLETHETA.dX)*cos(ANGLETHETA.dY);
	CIMatrix[6] = cos(ANGLETHETA.dX)*sin(ANGLETHETA.dY)*cos(ANGLETHETA.dZ) + sin(ANGLETHETA.dX)*sin(ANGLETHETA.dZ);
	CIMatrix[7] = cos(ANGLETHETA.dX)*sin(ANGLETHETA.dY)*sin(ANGLETHETA.dZ) - sin(ANGLETHETA.dX)*cos(ANGLETHETA.dZ);
	CIMatrix[8] = cos(ANGLETHETA.dX)*cos(ANGLETHETA.dY);

	//seneor to image coordinate system which could be modified according to different seneor
	ICMatrix[0] = 0; 	ICMatrix[1] = -1;	ICMatrix[2] = 0;
	ICMatrix[3] = -1;	ICMatrix[4] = 0;	ICMatrix[5] = 0;
	ICMatrix[6] = 0;	ICMatrix[7] = 0;	ICMatrix[8] = -1;

	double IMMatrix[9];
	double M1[9], M2[9], M3[9];
	double pVector[] = { 0,0,0 };

	MatrixMuti(EMMatrix, 3, 3, 3, EGMatrix, M1);
	MatrixMuti(M1, 3, 3, 3, GIMatrix, M2);
	MatrixMuti(M2, 3, 3, 3, CIMatrix, M3);
	MatrixMuti(M3, 3, 3, 3, ICMatrix, IMMatrix);

	double dPhi, dOmega, dKappa;
	dPhi = asin(-IMMatrix[2]);
	dOmega = atan(-IMMatrix[5] / IMMatrix[8]);
	if (nQuadNum == 1 || nQuadNum == 2)
	{
		dKappa = abs(atan(-IMMatrix[1] / IMMatrix[0]));
	}
	else
	{
		dKappa = (atan(-IMMatrix[1] / IMMatrix[0])) + PI;
	}

	//���㾵ͷ͸�����ĵ�λ��,��������ת��Ϊ��������ϵ������ͼ����ϵm������
	THREEDPOINT curPoint; 
	CoordinateTransBasic m_tmpCoordinate;
	m_tmpCoordinate.BLHToXYZ(dB, dL, dH, curPoint);
	double dXs = (curPoint.dX - XYZPoint.dX)*EMMatrix[0] + (curPoint.dY - XYZPoint.dY)*EMMatrix[1] + (curPoint.dZ - XYZPoint.dZ)*EMMatrix[2];
	double dYs = (curPoint.dX - XYZPoint.dX)*EMMatrix[3] + (curPoint.dY - XYZPoint.dY)*EMMatrix[4] + (curPoint.dZ - XYZPoint.dZ)*EMMatrix[5];
	double dZs = (curPoint.dX - XYZPoint.dX)*EMMatrix[6] + (curPoint.dY - XYZPoint.dY)*EMMatrix[7] + (curPoint.dZ - XYZPoint.dZ)*EMMatrix[8];

	double Muvw[9] = { POSISTTHETA.dX,POSISTTHETA.dY,POSISTTHETA.dZ }, MXTZ[3] = { 0.0,0.0,0.0 }, MXYZ[3] = { 0.0,0.0,0.0 };
	//double Muvw[9]={0,0.4,1.3},MXTZ[3];
	//double MXYZ[9]={POSISTTHETA.dX,POSISTTHETA.dY,POSISTTHETA.dZ}, MXTZ[3]={0.0,0.0,0.0};
	//double MXYZ[9]={-7.9345,-9.2496,-0.6722},MXTZ[3]={0.0,0.0,0.0};
	MatrixMuti(M3, 3, 3, 1, Muvw, MXTZ);
	dXs = dXs + MXTZ[0] + MXYZ[0];
	dYs = dYs + MXTZ[1] + MXYZ[1];
	dZs = dZs + MXTZ[2] + MXYZ[2];
	EO tempEO;
	tempEO.m_dX = dXs;
	tempEO.m_dY = dYs;
	tempEO.m_dZ = dZs;

	tempEO.m_phia = dPhi;
	tempEO.m_omega = dOmega;
	tempEO.m_kappa = dKappa;

	//double tMatrix[9];
	//MatrixRotate(tMatrix,dPhi,dOmega,dKappa);

	for (int n = 0; n<9; n++)
		tempEO.m_dRMatrix[n] = IMMatrix[n];
	m_Geo_EO.push_back(tempEO);

	if (fEO != NULL)
	{
		fprintf(fEO, "\n%20.10f	%20.10f	%20.10f	%15.10f	%15.10f	%15.10f ", dXs, dYs, dZs, dPhi, dOmega, dKappa);
		for (int n = 0; n<9; n++)
			fprintf(fEO, "	%20.10f", IMMatrix[n]);
	}
	return 0;
}

long GeoPOSProcess::GeoPOSProc_EOMatrixTurn(POS pdfPOS, THREEDPOINT &XYZPoint, THREEDPOINT THETA,float* pVector ,int nQuadNum, double EMMatrix[], EO &pdfEO)
{
	//rotate matrix 
	double EGMatrix[9];	//	
	double GIMatrix[9];	//
	double CIMatrix[9];	//
	double ICMatrix[9];	//

	double dL, dB, dH;
	double roll, pitch, yaw;
	dB =/*pfPOS[6*curpos+0]*/pdfPOS.m_latitude;
	dL =/*pfPOS[6*curpos+1]*/pdfPOS.m_longitude;
	dH =/*pfPOS[6*curpos+2]*/pdfPOS.m_height;

	roll =/*pfPOS[6*curpos+3]*/pdfPOS.m_roll;
	pitch =/*pfPOS[6*curpos+4]*/pdfPOS.m_pitch;
	yaw =/*pfPOS[6*curpos+5]*/pdfPOS.m_yaw;

	//WGS84 trans to local coordinate system
	EGMatrix[0] = -sin(dB)*cos(dL); EGMatrix[1] = -sin(dL); EGMatrix[2] = -cos(dB)*cos(dL);
	EGMatrix[3] = -sin(dB)*sin(dL); EGMatrix[4] = cos(dL); EGMatrix[5] = -cos(dB)*sin(dL);
	EGMatrix[6] = cos(dB);		 EGMatrix[7] = 0; ; EGMatrix[8] = -sin(dB);

	//Local coordinate system to IMU coordinate system
	GIMatrix[0] = cos(pitch)*cos(yaw);
	GIMatrix[1] = sin(roll)*sin(pitch)*cos(yaw) - cos(roll)*sin(yaw);
	GIMatrix[2] = cos(roll)*sin(pitch)*cos(yaw) + sin(roll)*sin(yaw);
	GIMatrix[3] = cos(pitch)*sin(yaw);
	GIMatrix[4] = sin(roll)*sin(pitch)*sin(yaw) + cos(roll)*cos(yaw);
	GIMatrix[5] = cos(roll)*sin(pitch)*sin(yaw) - sin(roll)*cos(yaw);
	GIMatrix[6] = -sin(pitch);
	GIMatrix[7] = sin(roll)*cos(pitch);
	GIMatrix[8] = cos(roll)*cos(pitch);

	//IMU to sensor coordinate system trans
	CIMatrix[0] = cos(THETA.dY)*cos(THETA.dZ);
	CIMatrix[1] = cos(THETA.dY)*sin(THETA.dZ);
	CIMatrix[2] = -sin(THETA.dY);
	CIMatrix[3] = sin(THETA.dX)*sin(THETA.dY)*cos(THETA.dZ) - cos(THETA.dX)*sin(THETA.dZ);
	CIMatrix[4] = sin(THETA.dX)*sin(THETA.dY)*sin(THETA.dZ) + cos(THETA.dX)*cos(THETA.dZ);
	CIMatrix[5] = sin(THETA.dX)*cos(THETA.dY);
	CIMatrix[6] = cos(THETA.dX)*sin(THETA.dY)*cos(THETA.dZ) + sin(THETA.dX)*sin(THETA.dZ);
	CIMatrix[7] = cos(THETA.dX)*sin(THETA.dY)*sin(THETA.dZ) - sin(THETA.dX)*cos(THETA.dZ);
	CIMatrix[8] = cos(THETA.dX)*cos(THETA.dY);

	//seneor to image coordinate system which could be modified according to different seneor
	ICMatrix[0] = 0; 	ICMatrix[1] = -1;	ICMatrix[2] = 0;
	ICMatrix[3] = -1;	ICMatrix[4] = 0;	ICMatrix[5] = 0;
	ICMatrix[6] = 0;	ICMatrix[7] = 0;	ICMatrix[8] = -1;

	double IMMatrix[9];
	double M1[9], M2[9], M3[9];
	

	MatrixMuti(EMMatrix, 3, 3, 3, EGMatrix, M1);
	MatrixMuti(M1, 3, 3, 3, GIMatrix, M2);
	MatrixMuti(M2, 3, 3, 3, CIMatrix, M3);
	MatrixMuti(M3, 3, 3, 3, ICMatrix, IMMatrix);

	double dPhi, dOmega, dKappa;

	dPhi = asin(-IMMatrix[2]);
	dOmega = atan(-IMMatrix[5] / IMMatrix[8]);
	if (nQuadNum == 1 || nQuadNum == 2)
	{
		dKappa = abs(atan(-IMMatrix[1] / IMMatrix[0]));
	}
	else
	{
		dKappa = abs(atan(-IMMatrix[1] / IMMatrix[0])) - PI / 2;
	}

	//get sensor center pos in the 
	THREEDPOINT curPoint;
	CoordinateTransBasic m_coordiTrans;
	m_coordiTrans.BLHToXYZ(dB, dL, dH, curPoint);
	double dXs = (curPoint.dX - XYZPoint.dX)*EMMatrix[0] + (curPoint.dY - XYZPoint.dY)*EMMatrix[1] + (curPoint.dZ - XYZPoint.dZ)*EMMatrix[2];
	double dYs = (curPoint.dX - XYZPoint.dX)*EMMatrix[3] + (curPoint.dY - XYZPoint.dY)*EMMatrix[4] + (curPoint.dZ - XYZPoint.dZ)*EMMatrix[5];
	double dZs = (curPoint.dX - XYZPoint.dX)*EMMatrix[6] + (curPoint.dY - XYZPoint.dY)*EMMatrix[7] + (curPoint.dZ - XYZPoint.dZ)*EMMatrix[8];

	// calculate the placement vector
	double transMatrix[9];
	MatrixMuti(EGMatrix, 3, 3, 3, GIMatrix, transMatrix);
	double dXl = transMatrix[0] * pVector[0] + transMatrix[1] * pVector[1] + transMatrix[2] * pVector[2];
	double dYl = transMatrix[3] * pVector[0] + transMatrix[4] * pVector[1] + transMatrix[5] * pVector[2];
	double dZl = transMatrix[6] * pVector[0] + transMatrix[7] * pVector[1] + transMatrix[8] * pVector[2];

	pdfEO.m_dX = dXs + dXl;
	pdfEO.m_dY = dYs + dYl;
	pdfEO.m_dZ = dZs + dZl;
	pdfEO.m_phia = dPhi;
	pdfEO.m_omega = dOmega;
	pdfEO.m_kappa = dKappa;

	//double tMatrix[9];
	//MatrixRotate(tMatrix,dPhi,dOmega,dKappa);

	for (int n = 0; n<9; n++)
		pdfEO.m_dRMatrix[n] = IMMatrix[n];

	return 0;
}

//���ļ��ж�ȡEOԪ��
long GeoPOSProcess::GeoPOSProc_ReadEOFile(const char* pathEO, double &dB, double &dL, double &dH)
{
	FILE *fp = NULL;
	errno_t err = 0;
	m_Geo_EO.clear();
	err=fopen_s(&fp,pathEO, "r");
	if (err!=0)
		return -1;

	int nImgLines = 0;
	int nPOSSamples;
	double dLatitude, dLongitude, dFlightHeight;
	double pdEOData[15];
	char szTemp[256];
	fscanf_s(fp, "%s%d", szTemp,256, &nImgLines);
	fscanf_s(fp, "%s%d", szTemp, 256, &nPOSSamples);
	fscanf_s(fp, "%s%lf", szTemp, 256, &dLatitude);
	fscanf_s(fp, "%s%lf", szTemp, 256, &dLongitude);
	fscanf_s(fp, "%s%lf", szTemp, 256, &dFlightHeight);
	fscanf_s(fp, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s", szTemp, 256, szTemp, 256, szTemp, 256, szTemp, 256, szTemp, 256, szTemp
		, 256, szTemp, 256, szTemp, 256, szTemp, 256, szTemp, 256, szTemp, 256, szTemp, 256, szTemp, 256, szTemp, 256, szTemp, 256);

	int nEOLines = nImgLines*nPOSSamples;

	for (int i = 0; i<nEOLines; i++)
	{
		EO tempEO;
		fscanf_s(fp, "%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf",
			pdEOData, pdEOData + 1, pdEOData + 2, pdEOData + 3, pdEOData + 4,
			pdEOData + 5, pdEOData + 6, pdEOData + 7, pdEOData + 8, pdEOData + 9,
			pdEOData + 10, pdEOData + 11, pdEOData + 12, pdEOData + 13, pdEOData + 14);

		tempEO.m_dX = pdEOData[0];
		tempEO.m_dY = pdEOData[1];
		tempEO.m_dZ = pdEOData[2];
		tempEO.m_phia = pdEOData[3];
		tempEO.m_omega = pdEOData[4];
		tempEO.m_kappa = pdEOData[5];
		for (int j = 0; j<9; j++)
			tempEO.m_dRMatrix[j] = pdEOData[j + 6];
		m_Geo_EO.push_back(tempEO);
	}
	dL = dLongitude;
	dB = dLatitude;
	dH = dFlightHeight;
	fclose(fp);



	return 0;
}
//=================================================================================================================================================
//��ȡ����POS����
long QPDGeoPOSProcess::GeoPOSProc_ReadPartPOS(const char* pPOSFile,int nLines,int nBeginLine)
{
	long   lError = 0;
	int    i;
	FILE   *fpin = NULL;
	double fReadData[20];
	char   cTempchar[2048];
	int realLines = 0;
	m_Geo_Pos.clear();
	if ((fopen_s(&fpin,pPOSFile, "r")) !=0 )
	{
		lError = -1;  //��POS�ļ�ʧ��
		goto ErrEnd;
	}

	m_Center_Geo_dB = m_Center_Geo_dL= m_Center_Geo_dH= 0.0;

	for (i = 0; i < nBeginLine; i++)
	{
		if (feof(fpin))
		{
			lError = -1; //��ȡPOS�ļ�ʧ��
			goto ErrEnd;
		}
		fgets(cTempchar, 2048, fpin);
	}

	//ʵ�ʵľ�γ�ȶ����Ի�����ʽ������
	for (i = 0; i < nLines&&!feof(fpin); i++)
	{
		realLines++;
		if (feof(fpin))
		{
			lError = 40308; //��ȡPOS�ļ�ʧ��
			goto ErrEnd;
		}
		fgets(cTempchar, 2048, fpin);
		lError = sscanf_s(
			cTempchar,
			"%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf",
			&fReadData[0], &fReadData[1], &fReadData[2], &fReadData[3],
			&fReadData[4], &fReadData[5], &fReadData[6], &fReadData[7],
			&fReadData[8], &fReadData[9], &fReadData[10], &fReadData[11],
			&fReadData[12], &fReadData[13], &fReadData[14], &fReadData[15], &fReadData[16]);
		if (lError == 0)
		{
			lError = -1; //����POS�ļ�ʧ��
			goto ErrEnd;
		}

		if (fReadData[2]*180/PI < -180.0 || fReadData[2] * 180 / PI > 180.0)
		{
			lError = 40316;//���������⣬����-180��180��֮��
			goto ErrEnd;
		}
		if (fReadData[1] * 180 / PI <= -90.0 || fReadData[1] * 180 / PI >= 90.0)
		{
			lError = 40317;//γ�������⣬����-90��90��֮��
			goto ErrEnd;
		}
		m_Center_Geo_dB += fReadData[1];
		m_Center_Geo_dL += fReadData[2];
		m_Center_Geo_dH += fReadData[3];
		POS tempPos;
		tempPos.m_latitude = double(fReadData[1]);				 //X
		tempPos.m_longitude = double(fReadData[2]);				 //Y
		tempPos.m_height = double(fReadData[3]);                 //height
		tempPos.m_roll = double(fReadData[4]);					 //roll
		tempPos.m_pitch = double(fReadData[5]);					 //pitch
		tempPos.m_yaw = double(fReadData[6]);					 //heading
		m_Geo_Pos.push_back(tempPos);
	}
	m_Center_Geo_dB /= nLines; /*dB = dB*PI/180.0;*/
	m_Center_Geo_dL /= nLines; /*dL = dL*PI/180.0;*/
	m_Center_Geo_dH /= nLines; ;

ErrEnd:
	if (fpin)
	{
		fclose(fpin);
		fpin = NULL;
	}
	return 0;
}

//��SBET�ļ��н���POS����
//SBET�ļ�Ϊ�������ļ���ÿһ����¼��¼�˸���GPS��IMU����Ϣ
//SBET�ļ���¼��Ϣ��Ƶ��ҪԶ����Ӱ�����Ƶ�ʣ������ʵ�ʴ�����������Ҫ����
//Ӱ��ûһ�г���ʱ��EVENT��Ϣ����SBET��Ϣ�н��в�ֵ��ȡӰ��ʵ��ʱ�̵�λ�ú���̬
long QPDGeoPOSProcess::GeoPOSProc_ExtractSBET(const char* pathSBET, const char* pathEvent, const char* pathPOS, float fOffsetGPS)
{
	long lError = 0;
	FILE *fSbet = NULL, *fEvent = NULL, *fPos = NULL;
	errno_t err = 0;

	//��SBET�ļ���EVENT�ļ���POS�ļ�
	err = fopen_s(&fSbet, pathSBET, "rb");		//������
	if (err)
	{
		lError = -1;
	}
	err = fopen_s(&fEvent, pathEvent, "r");		//������
	if (err)
	{
		lError = -1;
	}
	err = fopen_s(&fPos, pathPOS, "w");		//������
	if (err)
	{
		lError = -1;
	}

	//д���һ�б���
	fprintf(fPos, "%s\n", "SBET�ļ�����POS�ļ�");

	//д��ÿ�в�����Ϣ
	//��ʽ
	fprintf(fPos, "%18s", "Time(s)");
	fprintf(fPos, "%18s", "Latitude(rad)");
	fprintf(fPos, "%18s", "Longitude(rad)");
	fprintf(fPos, "%18s", "Altitude(m)");

	fprintf(fPos, "%18s", "Roll(rad)");
	fprintf(fPos, "%18s", "Pitch(rad)");
	fprintf(fPos, "%18s", "PlatHeading(rad)");
	fprintf(fPos, "%18s", "WanderAngle(rad)");

	fprintf(fPos, "%18s", "X_Velocity(m/s)");
	fprintf(fPos, "%18s", "Y_Velocity(m/s)");
	fprintf(fPos, "%18s", "Z_Velocity(m/s)");

	fprintf(fPos, "%22s", "X_Acceralation(m/s/s)");
	fprintf(fPos, "%22s", "Y_Acceralation(m/s/s)");
	fprintf(fPos, "%22s", "Z_Acceralation(m/s/s)");

	fprintf(fPos, "%22s", "X_Angual Rate(rad/s)");
	fprintf(fPos, "%22s", "Y_Angual Rate(rad/s)");
	fprintf(fPos, "%22s", "Z_Angual Rate(rad/s)");

	int ss = 0;
	double t, t1, t2, a, b;
	SBETELEMENT stLine, stFormerLine, stLatterLine;
	//	char tmp[256];

	//	fgets(tmp, 256, fEvent);	//��ȡ��һ����Ϣ
	_fseeki64(fSbet, 0, SEEK_SET);
	fread(&stLatterLine, sizeof(SBETELEMENT), 1, fSbet);	//��ȡSBET�ļ�

	stFormerLine.dGpsTime = stLatterLine.dGpsTime;
	stFormerLine.dLatitude = stLatterLine.dLatitude;
	stFormerLine.dLongitude = stLatterLine.dLongitude;
	stFormerLine.dHeight = stLatterLine.dHeight;

	stFormerLine.dRoll = stLatterLine.dRoll;
	stFormerLine.dPitch = stLatterLine.dPitch;
	stFormerLine.dHeading = stLatterLine.dHeading;
	stFormerLine.dWander = stLatterLine.dWander;

	stFormerLine.dVx = stLatterLine.dVx;
	stFormerLine.dVy = stLatterLine.dVy;
	stFormerLine.dVz = stLatterLine.dVz;

	stFormerLine.dAx = stLatterLine.dAx;
	stFormerLine.dAy = stLatterLine.dAy;
	stFormerLine.dAz = stLatterLine.dAz;

	stFormerLine.dArx = stLatterLine.dArx;
	stFormerLine.dAry = stLatterLine.dAry;
	stFormerLine.dArz = stLatterLine.dArz;

	int i = 0;
	while (!feof(fEvent))
	{
		//	fgets(tmp, 256, fEvent);			//��ȡ��һ��EVENT�ļ�
		fscanf_s(fEvent, "%d	%lf", &ss, &t);
		t += fOffsetGPS;
		i++;
		while (!feof(fSbet))
		{
			fread(&stLatterLine, sizeof(SBETELEMENT), 1, fSbet);	//��ȡ��һ��SBET�ļ�
			t1 = stFormerLine.dGpsTime;
			t2 = stLatterLine.dGpsTime;

			if (t >= t1 && t <= t2)
			{
				//t1<t<t2�����ҵ������У����м�Ȩ
				a = (t2 - t) / (t2 - t1);
				b = (t - t1) / (t2 - t1);

				stLine.dGpsTime = t;

				stLine.dLatitude = (a*stFormerLine.dLatitude + b*stLatterLine.dLatitude);
				stLine.dLongitude = (a*stFormerLine.dLongitude + b*stLatterLine.dLongitude);
				stLine.dHeight = a*stFormerLine.dHeight + b*stLatterLine.dHeight;

				stLine.dVx = a*stFormerLine.dVx + b*stLatterLine.dVx;
				stLine.dVy = a*stFormerLine.dVy + b*stLatterLine.dVy;
				stLine.dVz = a*stFormerLine.dVz + b*stLatterLine.dVz;

				stLine.dRoll = (a*stFormerLine.dRoll + b*stLatterLine.dRoll);
				stLine.dPitch = (a*stFormerLine.dPitch + b*stLatterLine.dPitch);
				stLine.dHeading = (a*stFormerLine.dHeading + b*stLatterLine.dHeading);
				stLine.dWander = (a*stFormerLine.dWander + b*stLatterLine.dWander);

				stLine.dAx = a*stFormerLine.dAx + b*stLatterLine.dAx;
				stLine.dAy = a*stFormerLine.dAy + b*stLatterLine.dAy;
				stLine.dAz = a*stFormerLine.dAz + b*stLatterLine.dAz;

				stLine.dArx = a*stFormerLine.dArx + b*stLatterLine.dArx;
				stLine.dAry = a*stFormerLine.dAry + b*stLatterLine.dAry;
				stLine.dArz = a*stFormerLine.dArz + b*stLatterLine.dArz;
				//д����ȡ��POS����
				fprintf(fPos, "\n%18.10f", stLine.dGpsTime);

				fprintf(fPos, "%18.10f", stLine.dLatitude);
				fprintf(fPos, "%18.10f", stLine.dLongitude);
				fprintf(fPos, "%18.10f", stLine.dHeight);

				fprintf(fPos, "%18.10f", stLine.dRoll);
				fprintf(fPos, "%18.10f", stLine.dPitch);
				fprintf(fPos, "%18.10f", stLine.dHeading);
				fprintf(fPos, "%18.10f", stLine.dWander);

				fprintf(fPos, "%18.10f", stLine.dVx);
				fprintf(fPos, "%18.10f", stLine.dVy);
				fprintf(fPos, "%18.10f", stLine.dVz);

				fprintf(fPos, "%22.10f", stLine.dAx);
				fprintf(fPos, "%22.10f", stLine.dAy);
				fprintf(fPos, "%22.10f", stLine.dAz);

				fprintf(fPos, "%22.10f", stLine.dArx);
				fprintf(fPos, "%22.10f", stLine.dAry);
				fprintf(fPos, "%22.10f", stLine.dArz);

				break;
			}
			else
			{//δ�ҵ������У������ҵĺ�һ��ֵ����ǰһ��
				stFormerLine.dGpsTime = stLatterLine.dGpsTime;
				stFormerLine.dLatitude = stLatterLine.dLatitude;
				stFormerLine.dLongitude = stLatterLine.dLongitude;
				stFormerLine.dHeight = stLatterLine.dHeight;

				stFormerLine.dRoll = stLatterLine.dRoll;
				stFormerLine.dPitch = stLatterLine.dPitch;
				stFormerLine.dHeading = stLatterLine.dHeading;
				stFormerLine.dWander = stLatterLine.dWander;

				stFormerLine.dVx = stLatterLine.dVx;
				stFormerLine.dVy = stLatterLine.dVy;
				stFormerLine.dVz = stLatterLine.dVz;

				stFormerLine.dAx = stLatterLine.dAx;
				stFormerLine.dAy = stLatterLine.dAy;
				stFormerLine.dAz = stLatterLine.dAz;

				stFormerLine.dArx = stLatterLine.dArx;
				stFormerLine.dAry = stLatterLine.dAry;
				stFormerLine.dArz = stLatterLine.dArz;
			}
		}
	}

	fclose(fSbet);
	fclose(fEvent);
	fclose(fPos);
	return lError;
}

//��ȡEOԪ�ص�����
long QPDGeoPOSProcess::GeoPOSProc_GetEOLines(const char *pEoFile, int &nEOLines)
{
	long lError = 0;
	FILE *fEO = NULL;
	if(fopen_s(&fEO,pEoFile, "r")!=0)			//���ļ�
	{
		lError = -1;
		goto ErrEnd;
	}
	char s[50];
	fscanf_s(fEO, "%s%d", s,50, &nEOLines);
ErrEnd:
	if (fEO)
	{
		fclose(fEO);
		fEO = NULL;
	}
	return lError;
}
//=================================================================================================================================================
long UAVGeoPOSProcess::GeoPOSProc_SetPOSFormat(char* formatStr,bool arc)
{
	m_arc = arc;
	formatVec.clear();
	char *p;
	p = strtok(formatStr,",");
	while(p)
	{
		if(!strcmp(p,"dB")||!strcmp(p,"latitude"))
			formatVec.push_back(1);
		else if(!strcmp(p,"dL")||!strcmp(p,"longitude"))
			formatVec.push_back(2);
		else if(!strcmp(p,"dH")||!strcmp(p,"height"))
			formatVec.push_back(3);
		else if(!strcmp(p,"roll"))
			formatVec.push_back(4);
		else if(!strcmp(p,"pitch"))
			formatVec.push_back(5);
		else if(!strcmp(p,"yaw"))
			formatVec.push_back(6);
		else
			formatVec.push_back(0);
		p=strtok(NULL,",");
	}
	return 0;
}

long UAVGeoPOSProcess::GeoPOSProc_ReadPartPOS(const char* pPOSFile,int nLines,int nBeginLine)
{
	if(formatVec.empty())
	{
		printf("please set POS format first\n");
		return -1;
	}
	
	FILE   *fpin = NULL;
	double fReadData[20];
	double fGetData[20];
	char   cTempchar[2048];
	int    item_pos_index = 0;
	const char *d = " ";	//�ո��ַ�
	char *p;

	if ((fopen_s(&fpin,pPOSFile, "r")) !=0 )
		return -1;

	m_Center_Geo_dB = m_Center_Geo_dL= m_Center_Geo_dH= 0.0;
	for (int i = 0; i < nBeginLine; i++)
	{
		if (feof(fpin))
			return -1;
		fgets(cTempchar, 2048, fpin);
	}
	//ʵ�ʵľ�γ�ȶ����Ի�����ʽ������
	for (int i = 0; i < nLines&&!feof(fpin); i++)
	{
		if (feof(fpin))
			return -1;
		fgets(cTempchar, 2048, fpin);

		//����POS�����е�ÿһ��
		item_pos_index = 0;
		p = strtok(cTempchar,d);
		while(p)
		{
			fGetData[item_pos_index] = atof(p);
			item_pos_index++;
			p=strtok(NULL,d);
		}
		//�ֱ��ȡÿһ����������
		for(int j=0;j<(int)formatVec.size();++j)
		{
			if(formatVec[j]==0)
				continue;
			else
				fReadData[formatVec[j]]=fGetData[j+1];//��һ���Ǳ��
		}
		if(!m_arc)
		{
			fReadData[1]*=PI/180;
			fReadData[2]*=PI/180;
			fReadData[4]*=PI/180;
			fReadData[5]*=PI/180;
			fReadData[6]*=PI/180;
		}
		if (fReadData[2]*180/PI < -180.0 || fReadData[2] * 180 / PI > 180.0)
			return -1;
		if (fReadData[1] * 180 / PI <= -90.0 || fReadData[1] * 180 / PI >= 90.0)
			return -1;


		POS tempPos;
		m_Center_Geo_dB += fReadData[1];
		m_Center_Geo_dL += fReadData[2];
		m_Center_Geo_dH += fReadData[3];
		tempPos.m_latitude = double(fReadData[1]);				 //X
		tempPos.m_longitude = double(fReadData[2]);				 //Y
		tempPos.m_height = double(fReadData[3]);                 //height
		tempPos.m_roll = double(fReadData[4]);					 //roll
		tempPos.m_pitch = double(fReadData[5]);					 //pitch
		tempPos.m_yaw = double(fReadData[6]);					 //heading
		m_Geo_Pos.push_back(tempPos);
	}
	m_Center_Geo_dB /= nLines; /*dB = dB*PI/180.0;*/
	m_Center_Geo_dL /= nLines; /*dL = dL*PI/180.0;*/
	m_Center_Geo_dH /= nLines; ;

	if (fpin)
	{
		fclose(fpin);
		fpin = NULL;
	}
	return 0;
}

long UAVGeoPOSProcess::GeoPOSProc_ExtractEO(POS m_perpos, EO &m_pereo)
{
	long lError = 0;

	THREEDPOINT theta;
	double dB, dL, dH;

	//get BLH value
	dB = m_perpos.m_latitude; dL = m_perpos.m_longitude; dH = m_perpos.m_height;
	double EMMatrix[9];
	EMMatrix[0] = -sin(dL);
	EMMatrix[1] = cos(dL);
	EMMatrix[2] = 0;

	EMMatrix[3] = -sin(dB)*cos(dL);
	EMMatrix[4] = -sin(dB)*sin(dL);
	EMMatrix[5] = cos(dB);

	EMMatrix[6] = cos(dB)*cos(dL);
	EMMatrix[7] = cos(dB)*sin(dL);
	EMMatrix[8] = sin(dB);

	//coordinatetrans from BLH to XYZ
	THREEDPOINT XYZPnt;
	CoordinateTransBasic m_coordTrans;
	m_coordTrans.BLHToXYZ(dB, dL, 0, XYZPnt);		//BL0->XYZ
	int nQuandNum = GeoPOSProc_EOQuadrant(m_perpos, EMMatrix, XYZPnt);

	//���˻�Ӱ�񲻿��ǰ��ýǺͰ���ʸ��
	theta.dX = theta.dY = theta.dZ = 0;	//do not take the palcement angle into consideration
	float pVector[3] = { 0.0,0.0,0.0 };//do not take the palcement vector into consideration

	if (GeoPOSProc_EOMatrixTurn(m_perpos, XYZPnt, theta,pVector, nQuandNum, EMMatrix, m_pereo) == 0)
		return -1;
	else
		return 0;

	//���˻��õ��ľ����ⷽλԪ�ز���Ҫ����ת����?
	//m_pereo.m_dX	=m_pereo.m_dY=0;
	//m_pereo.m_dZ	=m_perpos.m_height;
	//m_pereo.m_phia	=-m_perpos.m_roll;
	//m_pereo.m_omega	=-m_perpos.m_pitch;
	//m_pereo.m_kappa	=PI-m_perpos.m_yaw;
	//MatrixRotate(m_pereo.m_dRMatrix,m_pereo.m_phia,m_pereo.m_omega,m_pereo.m_kappa);
	return 0;
}