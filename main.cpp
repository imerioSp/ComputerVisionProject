//OpenCV
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/features2d/features2d.hpp"

//Librerie di sistema
#include <iostream>
#include <string.h>
#include <fstream>

#define PATH "C:\\input\\"

using namespace cv;
using namespace std;

//Classi di utilità
class Foro
{

public:
	int id, name;//name e' il numero assegnato al foro dalla biella
	vector<Point> contours;
	double area = 0;
	double diameterSize = 0;
	Moments mu;
	Point2f mc;

	Foro(int idForo, vector<Point> cont)
	{
		id = idForo;
		contours = cont;
		area = contourArea(cont);
		diameterSize = arcLength(contours, true) / CV_PI;
	}

};

class Biella
{
public:

	int numberOfChild = 0;
	string tipoBiella;
	vector<Foro> fori;
	vector<Point> contours;
	int id;
	int name;//Avrà un nuovo "id" per ordinarlo
	double area = 0;
	double width = 0;
	double height = 0;
	double angle;
	double widthAtCentroid;
	Moments mu;
	Point2f mc;

	Biella(int idBiella, vector<Point> cont)
	{
		id = idBiella;
		contours = cont;
		area = contourArea(contours);
		vector<Foro> fori = vector<Foro>();

	}
	//Se il foro è molto simile a me vuol dire che non sono una biella ma una rondella!
	bool isRondella()
	{
		return (matchShapes(contours, fori[0].contours, 1, 0) < 0.5) ? true : false;
	}

	bool addChild(Foro foro)
	{
		for (Foro f : fori)//Se il foro da aggiungere già esiste esco
		{
			if (f.id == foro.id)
				return false;
		}
		numberOfChild++;
		foro.name = numberOfChild;
		fori.push_back(foro);
		area = area - contourArea(foro.contours);//Diminuisco l'area della biella per l'area del foro
		if (numberOfChild == 1)
			tipoBiella = "Tipo A";
		else
			tipoBiella = "Tipo B";
		return true;
	}

};


//Dichiarazione di funzioni implementate successivamente
bool exist(int k, vector<Biella> bielle);

String images[15] = { "TESI00.BMP", "TESI01.BMP", "TESI12.BMP", "TESI21.BMP", "TESI31.BMP", "Tesi33.bmp", "TESI44.BMP", "TESI47.BMP", "TESI48.BMP", "TESI49.BMP", "TESI50.BMP", "TESI51.BMP", "TESI90.BMP", "TESI92.BMP", "TESI98.BMP" };

int main(int argc, char** argv)
{
	Mat imageInput, imageInputGray, imageAfterOtsu, imageAfterMedianBlur;
	ofstream fileOutput;

	for (int i = 0; i<15; i++)
	{
		//apertura file che conterrà i log
		String file(String(PATH) + "output/" + images[i] + ".txt");
		fileOutput.open(file.c_str());

		//lettura immagine in formato RGB
		imageInput = imread(String(PATH) + images[i], CV_LOAD_IMAGE_COLOR);
		fileOutput << "Immagine: " << images[i] << "\n";

		//verifico l'esistenza dell'immagine
		if (!imageInput.data)
		{
			cout << "Impossibile aprire l'immagine" << std::endl;
			if (i < 15) continue; return -1;
		}

		//RGB->Livelli di grigio....necessario per OTSU
		cvtColor(imageInput, imageInputGray, CV_BGR2GRAY);

		//applico OTSU
		double otsu_thresh_val = threshold(imageInputGray, imageAfterOtsu, 0, 255, CV_THRESH_OTSU);
		//applico 4 volte un filtro mediano 3x3 per risolvere il problema della 'polvere'
		medianBlur(imageAfterOtsu, imageAfterMedianBlur, 3);
		medianBlur(imageAfterMedianBlur, imageAfterMedianBlur, 3);
		medianBlur(imageAfterMedianBlur, imageAfterMedianBlur, 3);
		medianBlur(imageAfterMedianBlur, imageAfterMedianBlur, 3);

		//labeling
		RNG rng(12345);

		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;

		//Se vi sono immagini con elementi "attaccati" qui li "stacco"
		Mat bw = imageAfterMedianBlur.clone();//Eseguo operazioni su una copia dell'immagine, poichè l'immagine mi servirà dopo
		bw = bw < 60;

		/// Find contours
		findContours(bw.clone(), contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0)); //serve la clone...dato che mi riservirà dopo

		/// Draw contours
		vector<int> contoursHull;
		vector<Vec4i> defects;
		vector<Point> points;

		for (size_t i = 0; i < contours.size(); i++)
		{
			if (contourArea(contours[i]) > 500)
			{
				approxPolyDP(contours[i], contours[i], 2, true);//Approssimazione della curva (Contorno) . 2 è la distanza massima fra la vecchia curva e quella approssimata
				convexHull(contours[i], contoursHull, true);
				convexityDefects(contours[i], contoursHull, defects);

				for (size_t j = 0; j < defects.size(); j++)
				{
					Vec4i defpoint = defects[j];
					Point pt = contours[i][defpoint[2]]; // get defect point
					Rect r3x3(pt.x - 2, pt.y - 2, 5, 5); // create 5x5 Rect from defect point

					// maybe no need but to be sure that the rect is in the image
					r3x3 = r3x3 & Rect(0, 0, bw.cols, bw.rows);

					if (countNonZero(bw(r3x3)) > 17)
						points.push_back(contours[i][defpoint[2]]);
				}
			}
		}
		for (Point p : points)
		{
			for (Point p2 : points)
			{
				double difference = sqrt((p.x - p2.x) *(p.x - p2.x) + (p.y - p2.y) *(p.y - p2.y));
				if (difference<30 && difference != 0)
				{
					//line effettiva
					line(imageAfterMedianBlur, p2, p, Scalar(255, 255, 255), 2, 8);
					p2.y = p2.y - 2;
					p.y = p.y - 2;
					line(imageAfterMedianBlur, p2, p, Scalar(255, 255, 255), 1, 8);
					p2.y = p2.y + 3;
					p.y = p.y + 3;
					p2.x = p2.x + 1;
					p.x = p.x + 1;
					//Aggiungo delle linee nere per riformare la biella deformata dalla linea bianca introdotta precedentemente
					line(imageAfterMedianBlur, p2, p, Scalar(0, 0, 0), 1, 8);
					p2.y = p2.y + 1;
					p.y = p.y + 1;
					line(imageAfterMedianBlur, p2, p, Scalar(0, 0, 0), 1, 8);
					p2.y = p2.y + 1;
					p.y = p.y + 1;
					line(imageAfterMedianBlur, p2, p, Scalar(0, 0, 0), 1, 8);
					p2.y = p2.y + 1;
				}
			}
		}

		//Ora tutti gli oggetti sono "staccati", inizio con l'analisi dell'immagine
		findContours(imageAfterMedianBlur.clone(), contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0)); //serve la clone...dato che mi riservirà dopo
		Mat drawing = Mat::zeros(imageAfterMedianBlur.size(), CV_8UC3);

		vector<Biella> bielle = vector<Biella>();
		for (int k = 1; k < contours.size(); k++)
		{
			//Se ho dei figli e mio padre e' 0
			if (hierarchy[k][2] != -1 && hierarchy[k][3] == 0)
			{
				//sono una biella, se non sono stata creata mi creo
				if (exist(k, bielle))
					continue;

				Biella biella = Biella(k, contours[k]);
				Foro foro = Foro(hierarchy[k][2], contours[hierarchy[k][2]]);//Creo il primo figlio
				biella.addChild(foro);
				bielle.push_back(biella);
			}
			else if (hierarchy[k][2] == -1 && hierarchy[k][3] != -1 && hierarchy[k][3] != 0)//Se non ho figli ma ho un padre
			{
				//Sono il figlio di una biella
				Foro foro = Foro(k, contours[k]);
				bool find = false;

				for (int ii = 0; ii < bielle.size(); ii++)//Se non sono stato inserito nella biella corretta mi inserisco
				{
					if (bielle[ii].id == hierarchy[k][3])
					{
						bielle[ii].addChild(foro);
						find = true;
						break;
					}
				}
				if (!find)//La biella padre non e' ancora stata creata, la creo ora
				{
					Biella biella = Biella(hierarchy[k][3], contours[hierarchy[k][3]]);
					biella.addChild(foro);
					bielle.push_back(biella);
				}
			}
		}

		RotatedRect minRect;
		Scalar color;
		Scalar fcolor;//colore per i fori
		for (int index = 0; index < bielle.size(); index++)
		{
			if (bielle[index].isRondella())//Se vi è un errore ed in realtà la biella è una rondella la si cancella
			{
				bielle.erase(bielle.begin() + index);
				index--;
				continue;
			}
			Biella biella = bielle[index];
			biella.name = index;

			//Disegno i contorni della biella e dei suoi fori
			color = Scalar(rng.uniform(50, 255), rng.uniform(50, 255), rng.uniform(50, 255));
			//imageInput->vado a disegnare sulla img iniziale/////drawing->vado a scrivere sulla img elaborata
			drawContours(imageInput, contours, biella.id, color, 1, 8, hierarchy, 0, Point());
			for (Foro foro : biella.fori)
			{
				fcolor = Scalar(rng.uniform(50, 255), rng.uniform(50, 255), rng.uniform(50, 255));
				drawContours(imageInput, contours, foro.id, fcolor, 1, 8, hierarchy, 0, Point());
			}

			//calcolo il baricentro
			biella.mu = moments(biella.contours, false);
			biella.mc = Point2f(biella.mu.m10 / biella.mu.m00, biella.mu.m01 / biella.mu.m00);

			//Disegno i baricentri
			Scalar bcolor = Scalar(rng.uniform(255, 255), rng.uniform(255, 255), rng.uniform(255, 255));
			circle(imageInput, biella.mc, 1, bcolor, -1, 8, 0);
			for (int k = 0; k < biella.fori.size(); k++)
			{
				biella.fori[k].mu = moments(biella.fori[k].contours, false);//calcolo baricentro del foro
				biella.fori[k].mc = Point2f(biella.fori[k].mu.m10 / biella.fori[k].mu.m00, biella.fori[k].mu.m01 / biella.fori[k].mu.m00);
				circle(imageInput, biella.fori[k].mc, 1, bcolor, -1, 8, 0);//disegno baricentro del foro
			}

			// Creo il rettangolo
			minRect = minAreaRect(Mat(biella.contours));

			// Disegno il rettangolo
			Point2f rect_points[4]; minRect.points(rect_points);
			for (int j = 0; j < 4; j++)
				line(imageInput, rect_points[j], rect_points[(j + 1) % 4], color, 1, 8);


			//Trovo la lunghezza e larghezza del rettangolo
			double lenA = sqrt((rect_points[0].x - rect_points[1].x)*(rect_points[0].x - rect_points[1].x) + (rect_points[0].y - rect_points[1].y)*(rect_points[0].y - rect_points[1].y));//rettangolo orizzontale
			double lenB = sqrt((rect_points[3].x - rect_points[0].x)*(rect_points[3].x - rect_points[0].x) + (rect_points[3].y - rect_points[0].y)*(rect_points[3].y - rect_points[0].y));//rettangolo verticale
			//Disegno la linea all'altezza del baricentro e calcolo la larghezza al baricentro
			Point leftStart, leftEnd, rightStart, rightEnd;
			Point pointArrow1;
			Point pointArrow2;
			//le rette che collegano i punti 0-1, e 2-3 del rettangolo corrispondono alla larghezza, rette che collegano i punti 1-2 e 3-0 corrispondono alla lunghezza
			if (lenA<lenB)
			{
				biella.width = lenA;
				biella.height = lenB;
				biella.angle = minRect.angle;

				leftStart = rect_points[1];
				leftEnd = rect_points[2];
				rightStart = rect_points[0];
				rightEnd = rect_points[3];
				
				pointArrow1 = Point(biella.mc.x + biella.height / 5 * cos(0), biella.mc.y + biella.height / 5 * sin(0));
				pointArrow2 = Point(biella.mc.x + (biella.height / 5) * cos((biella.angle)* CV_PI / 180.0), biella.mc.y + (biella.height / 5) * sin((biella.angle)* CV_PI / 180.0));

			}
			else//le rette che collegano i punti 0-1, e 2-3 del rettangolo corrispondono alla lunghezza, rette che collegano i punti 1-2 e 3-0 corrispondono alla larghezza
			{
				biella.width = lenB;
				biella.height = lenA;
				biella.angle = minRect.angle - 90;//ruoto di 90gradi l'angolo

				leftStart = rect_points[0];
				leftEnd = rect_points[1];
				rightStart = rect_points[3];
				rightEnd = rect_points[2];

				pointArrow1 = Point(biella.mc.x + biella.height / 5 * cos(0), biella.mc.y + biella.height / 5 * sin(0));
				pointArrow2 = Point(biella.mc.x + (biella.height / 5) * cos((biella.angle)* CV_PI / 180.0), biella.mc.y + (biella.height / 5) * sin((biella.angle)* CV_PI / 180.0));

			}

			ellipse(imageInput, biella.mc, Size(biella.height / 20, biella.height / 20), 0, 0, biella.angle, Scalar(0, 0, 255));//draw angle
			arrowedLine(imageInput, biella.mc, pointArrow1, Scalar(0, 0, 255)); // draw arrow!
			arrowedLine(imageInput, biella.mc, pointArrow2, Scalar(0, 0, 255)); // draw arrow!

			LineIterator itLeft(imageInput, leftStart, leftEnd, 8);
			LineIterator itRight(imageInput, rightStart, rightEnd, 8);

			Point p1, p2, p3;

			for (int z = 0; z < itLeft.count; z++, ++itLeft, ++itRight)
			{
				p1 = itLeft.pos();
				p2 = itRight.pos();
				//Equazione che dati due punti di un segmento ed un generico punto x, verifica se il punto x appartiene al segmento
				double res = abs((p2.y - p1.y) * biella.mc.x - (p2.x - p1.x) * biella.mc.y + p2.x * p1.y - p2.y * p1.x) /
					sqrt((p2.y - p1.y)*(p2.y - p1.y) + (p2.x - p1.x)*(p2.x - p1.x));
				if (res < 1)//Se il punto del baricentro appartiene alla retta
				{
					LineIterator iterator(imageInput, p1, p2, 8);
					Point point = Point(0, 0);

					for (int count = 0; count < iterator.count; count++, ++iterator)
					{
						double dist = pointPolygonTest(biella.contours, iterator.pos(), true);//Se il punto del segmento si trova sullo spigolo del contorno
						if (dist<1 && dist>-1)
						{
							int x = abs((iterator.pos().x - point.x));
							int y = abs((iterator.pos().y - point.y));
							if (x == 1 && y == 1 || x == 0 && y == 1 || x == 1 && y == 0)//se i due punti sono troppo vicini controllo il prossimo punto
								continue;
							if (point != Point(0, 0))
							{
								line(imageInput, point, iterator.pos(), color, 1, 8);
								biella.widthAtCentroid = sqrt((point.y - iterator.pos().y)*(point.y - iterator.pos().y) + (point.x - iterator.pos().x)*(point.x - iterator.pos().x));
								break;
							}
							else
								point = iterator.pos();
						}
					}
					break;
				}
			}

			fileOutput << "\nBiella: " << biella.name << " di " << biella.tipoBiella << "\t";
			fileOutput << "Area: " << biella.area << "\t";
			fileOutput << "Altezza: " << biella.height << "\t";
			fileOutput << "Larghezza: " << biella.width << "\t";
			fileOutput << "Orientamento: " << abs(biella.angle) << "°\t";
			fileOutput << "Posizione del baricentro: " << biella.mc << "\n";
			fileOutput << "Larghezza all'altezza del baricentro: " << biella.widthAtCentroid << "\n";

			for (Foro foro : biella.fori)
			{
				fileOutput << "Foro: " << foro.name << "\t";
				fileOutput << "Posizione del baricentro: " << foro.mc << "\t";
				fileOutput << "Dimensione del foro: " << foro.diameterSize << "\t";
			}
		}

		//scrittura immagine
		imwrite(string(PATH) + "output/" + images[i], imageInput);

		fileOutput.close();

	}

	waitKey(0);
	return 0;
}

bool exist(int k, vector<Biella> bielle)
{
	for (Biella biella : bielle)
	{
		if (biella.id == k)
		{
			return true;
		}
	}
	return false;
}