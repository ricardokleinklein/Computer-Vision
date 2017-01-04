 /* ************************** MAIN PROGRAM ***************************
  * Main program for the subject's project. It includes the main 
  * callings and features to proceed with the recognition of a 
  * spanish deck of cards. 
  *  */
  
  #include <iostream> 							// standard I/O stream
  #include <fstream>							// files I/O							
  #include <cstdlib>							// General op
  #include <cstdio>								// General op
  #include <string>								// strings of char
  #include "opencv2/core/core.hpp"				// basic methods OpenCV
  #include "opencv2/highgui/highgui.hpp"		// user interfase
  #include "opencv2/imgproc/imgproc.hpp"		// img processing
  
  using namespace std;
  using namespace cv;
  
  vector<Mat> loadDatabase();
  
  Mat preprocessing(Mat image);
  
  vector<vector<Point> > findCardContours(Mat image);
  
  vector<Mat> extractCard(vector<vector<Point> > contours,Mat image);
  
  Mat fitCard(Mat image);
  
  Mat binaryze(Mat image);
  
  int cmpCards(Mat image,vector<Mat> tmpImg);
  
  void printIDonImage(vector<int> ID,Mat image,vector<vector<Point> > contours);
  /* **************************************************************** */
  
  int main(int argc,char* argv[]){
	  
	  string nameUI = "Visualizacion";		// Name of the plot title
	  namedWindow(nameUI,WINDOW_AUTOSIZE);	// Open a window to show image
	  vector<Mat> dataBase = loadDatabase();	// Load the database
	  
	  string filename = "varios.jpg";		// Get the image file from current folder
	  Mat frame;
	  frame = imread(filename, CV_LOAD_IMAGE_COLOR);   // Read the file
	  resize(frame,frame,Size(0,0),0.25,0.25,INTER_LINEAR);		// Resize the picture 
	  
	  Mat procsImg = preprocessing(frame);	// Basic preparation of the scene
	  
	  vector<vector<Point> > cardContours = findCardContours(procsImg);	// Get the biggest contours of the figure
	  procsImg.release();
	  
	  vector<Mat> card = extractCard(cardContours,frame);	// Get individual cards as different images with black background
	  vector<int> cardID;
	  for (int i=0;i<cardContours.size();i++){
		  card[i] = fitCard(card[i]);			// Get a fixed-size and fixed-orientation individual card, cropped from the background
		  card[i] = binaryze(card[i]);			// Convert to a binary scale on contours
		  cardID.push_back(cmpCards(card[i],dataBase));	// Compare with teh database to identify the card
	  }
	  
	  printIDonImage(cardID,frame,cardContours);	// Print the results on screen
	  
	 imshow(nameUI,frame);
	 waitKey(0);
	 return 0;
  }
  
  
  /* **************************************************************** */

 Mat preprocessing(Mat image){
	 /* Converts to grayscale, adds Gaussian blur and an converts to 
	  * binary an image.
	  * */
	  Mat output;
	  cvtColor(image,output,CV_BGR2GRAY);		// Convert to grayscale
	  GaussianBlur(output,output,Size(5,5),0);	// Add gaussian blur to remove noise	
	  threshold(output,output,150,255,THRESH_BINARY);	// Binary image
	  return output;
  }
	  
 vector<vector<Point> > findCardContours(Mat image){
	 /* Returns the vector of biggest contours, which shall be cards.
	  * */
	  vector<vector<Point> > prevCont;
	  vector<Vec4i> hierarchy;
	  findContours(image,prevCont,hierarchy,CV_RETR_EXTERNAL,CV_CHAIN_APPROX_SIMPLE,Point());	// Get all the biggest contours
	  
	  vector<vector<Point> > output;
	  double avgRel = 50;		// Average relationship Area/Perimeter for a card from this view
	  for (int i=0;i<prevCont.size();i++){
		  double relAreaPer = contourArea(prevCont[i],false)/arcLength(prevCont[i],true);	// Relationship area/perimeter per object
		  if ((relAreaPer > 0.5*avgRel) && (relAreaPer < 2*avgRel)){		// Contidion over the relationship
			  output.push_back(prevCont[i]);		// Accepted if relationship is within the range
		  }	
	  }
	  return output;
  }
	  
 vector<Mat> extractCard(vector<vector<Point> > contours,Mat image){
	 /* Takes a contour and an image and creates a image with the 
	  * content within the contour and black background.
	  * */
	  int numCards = contours.size();
	  vector<Mat> output(numCards);
	  for (int i=0;i<numCards;i++){
		  Mat mask = Mat::zeros(image.rows,image.cols,CV_8UC1);
		  Mat crop;
		  RotatedRect rRect = minAreaRect(Mat(contours[i]));
		  double angle = rRect.angle;
		  if (rRect.size.width > rRect.size.height){
			angle += 90.;
		  }
		  Mat R = getRotationMatrix2D(rRect.center,angle,1.0);
		  drawContours(mask,contours,i,Scalar(255),CV_FILLED);
		  crop.setTo(Scalar(0));
		  image.copyTo(crop,mask);
		  normalize(mask.clone(),mask,0,255,CV_MINMAX,CV_8UC1);
		  warpAffine(crop,crop,R,image.size());
		  output[i] = crop;
	  }
	  return output;
 }
 
 Mat fitCard(Mat image){
	 /* Returns an array of Mat containing the cards from nearly
	  * above, using perspective transformations.
	  * */
	  Mat copy;image.copyTo(copy);		// Copy of the image to process
	  copy = preprocessing(copy);
	  vector<vector<Point> > contours;
	  findContours(copy,contours,CV_RETR_EXTERNAL,CV_CHAIN_APPROX_SIMPLE,Point());		// Find the new contours (1 found)
	  Mat output;
	  double perimeter = arcLength(contours[0],true);		// Perimeter of the contour
	  vector<vector<Point> > polygon(1);
	  approxPolyDP(Mat(contours[0]),polygon[0],0.01*perimeter,true);	// Polygon around the card
	  Rect boundRect = boundingRect(contours[0]);		// Bounding up-right rectangle around the card
	  Point2f target[4];		// Points to fit the card to
	  target[0] = Point2f(0,0);
	  target[1] = Point2f(449,0);
	  target[2] = Point2f(449,449);
	  target[3] = Point2f(0,449);
	  Point2f corner[4];		// Coordinates of the vertices of the polygon (provisional)
	  Point2f cornerOrdered[4];
	  corner[0] = Point2f(polygon[0][0].x,polygon[0][0].y);		// Get the corner catched by the polygon in the same order
	  corner[1] = Point2f(polygon[0][1].x,polygon[0][1].y);
	  corner[2] = Point2f(polygon[0][2].x,polygon[0][2].y);
	  corner[3] = Point2f(polygon[0][3].x,polygon[0][3].y);
	  Rect rect = boundingRect(polygon[0]);
	  double width = rect.width;
	  double height = rect.height;
	  // Find the closest corner to the upper-left corner of the screen
	  vector<bool> tookPoint(4);
	  tookPoint[0] = false;tookPoint[1] = false;
	  tookPoint[2]= false;tookPoint[3] = false;
	  Point2f screen = Point2f(0,0);
	  double minDist = 1e5;
	  double x;double y;
	  int idx;
	  for (int i=0;i<4;i++){
		  double dist = norm(Mat(screen),Mat(corner[i]));
		  if (tookPoint[i] == false){
			if (dist < minDist){minDist = dist;x = corner[i].x;y=corner[i].y;idx = i;}	// Update the min. distance and get the coordinates
		}
	  }
	  tookPoint[idx] = true;
	  cornerOrdered[0] = Point2f(x,y);		// Will be the upper-left corner
	  // ASUMIMOS QUE EL PUNTO DE VISTA ES DESDE ARRIBA, LUEGO LAS CARTAS LLEGAN VERTICALMENTE
	  Point2f ref[1];		// Reference point to specify the coordinates
	  ref[0] = Point2f(x+width,y);
	  minDist = 1e5;
	  for (int i=0;i<4;i++){
		  double dist = norm(Mat(ref[0]),Mat(corner[i]));
		  if (tookPoint[i] == false){
			if (dist < minDist){minDist = dist;x = corner[i].x;y=corner[i].y;idx = i;}	// Update the min. distance and get the coordinates
		}
	  }
	  tookPoint[idx] = true;
	  cornerOrdered[1] = Point2f(x,y);
	  
	  ref[0] = Point2f(x+width,y+height);
	  minDist = 1e5;
	  for (int i=0;i<4;i++){
		  double dist = norm(Mat(ref[0]),Mat(corner[i]));
		  if (tookPoint[i] == false){
			if (dist < minDist){minDist = dist;x = corner[i].x;y=corner[i].y;idx = i;}	// Update the min. distance and get the coordinates
		}
	  }
	  tookPoint[idx] = true;
	  cornerOrdered[2] = Point2f(x,y);
	  
	  ref[0] = Point2f(x-width,y+height);
	  minDist = 1e5;
	  for (int i=0;i<4;i++){
		  double dist = norm(Mat(ref[0]),Mat(corner[i]));
		  if (tookPoint[i] == false){
			if (dist < minDist){minDist = dist;x = corner[i].x;y=corner[i].y;idx = i;}	// Update the min. distance and get the coordinates
		}
	  }
	  tookPoint[idx] = true;
	  cornerOrdered[3] = Point2f(x,y);
	  
	  Mat T = getPerspectiveTransform(cornerOrdered,target);	// Get the homography
	  warpPerspective(image,output,T,image.size());	// Perform the transformation
	  
	  Rect rect2; rect2.x = 45; rect2.y = 25; rect2.width = 365; rect2.height = 400;
	  output = output(rect2);
	  return output;
  }

 Mat binaryze(Mat image){
	 /* Split the image in channels and calculates the histogram to 
	  * identify the card in case it is a number, not figure nor aces.
	  * */
	  // Number: Count detected contours that are under certain percent
	  // of the total area of the card.
	  // Kind: Color histogram comparison
	  Mat copy;image.copyTo(copy);
	  cvtColor(copy,copy,CV_BGR2HSV);
	  vector<Mat> channel(3);
	  split(copy,channel);
	  GaussianBlur(channel[1],channel[1],Size(25,25),0);
	  Mat output;
	  adaptiveThreshold(channel[1],output,255,ADAPTIVE_THRESH_MEAN_C,THRESH_BINARY,255,0);
	  GaussianBlur(output,output,Size(5,5),0);
	  return output;
 }

 int cmpCards(Mat image,vector<Mat> tmpImg){
	/* Compares a card with the database and returns the corresponding
	 * index. */
	 int tmpSize = tmpImg.size();
	 int minDist = 1e5;
	 int idx;
	 for (int i=0;i<tmpSize;i++){
		 Mat diff;
		 absdiff(image,tmpImg[i],diff);
		 int dist = countNonZero(diff);
		 if (dist < minDist){
			 minDist = dist;
			 idx = i;
		 }
	 }
	 return idx;
}
 
 vector<Mat> loadDatabase(){
	 /* Loads and stores all the cards in the database. */
	 vector<Mat> output;
	 // Oros
	 Mat prov = imread("binOros1.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//0
	 prov = imread("binOros2.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//1
	 prov = imread("binOros3.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//2	
	 prov = imread("binOros4.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//3	
	 prov = imread("binOros5.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//4
	 prov = imread("binOros6.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//5
	 prov = imread("binOros7.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//6
	 prov = imread("binOros10.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//7
	 prov = imread("binOros11.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//8
	 prov = imread("binOros12.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//9
	 
	 prov = imread("binOros1_Inv.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov); //10
	 prov = imread("binOros3_Inv.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//11	
	 prov = imread("binOros7_Inv.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//12
	 prov = imread("binOros10_Inv.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//13
	 prov = imread("binOros11_Inv.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov); //14
	 prov = imread("binOros12_Inv.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov); //15
	 
	// Copas
	 prov = imread("binCopas1.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//16
	 prov = imread("binCopas2.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//17
	 prov = imread("binCopas3.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//18
	 prov = imread("binCopas4.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//19
	 prov = imread("binCopas5.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//20
	 prov = imread("binCopas6.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//21
	 prov = imread("binCopas7.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);		//22
	 prov = imread("binCopas10.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//23
	 prov = imread("binCopas11.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//24
	 prov = imread("binCopas12.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//25
	 
	 prov = imread("binCopas1_Inv.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//26
	 prov = imread("binCopas2_Inv.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//27	
	 prov = imread("binCopas3_Inv.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//28
	 prov = imread("binCopas4_Inv.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//29
	 prov = imread("binCopas5_Inv.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//30
	 prov = imread("binCopas6_Inv.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//31
	 prov = imread("binCopas7_Inv.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//32
	 prov = imread("binCopas10_Inv.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);//33	
	 prov = imread("binCopas11_Inv.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);//34	
	 prov = imread("binCopas12_Inv.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);//35
	 
	 // Espadas
	 prov = imread("binEsp1.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//36
	 prov = imread("binEsp2.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//37
	 prov = imread("binEsp3.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//38
	 prov = imread("binEsp4.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//39
	 prov = imread("binEsp5.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//40
	 prov = imread("binEsp6.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//41	
	 prov = imread("binEsp7.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//42	
	 prov = imread("binEsp10.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//43
	 prov = imread("binEsp11.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//44
	 prov = imread("binEsp12.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//45
	 
	 prov = imread("binEsp1_Inv.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//46
	 prov = imread("binEsp3_Inv.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//47
	 prov = imread("binEsp5_Inv.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//48
	 prov = imread("binEsp7_Inv.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//49	
	 prov = imread("binEsp10_Inv.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//50
	 prov = imread("binEsp11_Inv.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//51
	 prov = imread("binEsp12_Inv.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//52
	 
	 // Bastos
	 prov = imread("binBastos1.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//53
	 prov = imread("binBastos2.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//54
	 prov = imread("binBastos3.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//55
	 prov = imread("binBastos4.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//56
	 prov = imread("binBastos5.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//57
	 prov = imread("binBastos6.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//58
	 prov = imread("binBastos7.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//59	
	 prov = imread("binBastos10.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//60
	 prov = imread("binBastos11.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//61
	 prov = imread("binBastos12.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//62
	 
	 prov = imread("binBastos1_Inv.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//63
	 prov = imread("binBastos3_Inv.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//64	
	 prov = imread("binBastos5_Inv.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//65
	 prov = imread("binBastos7_Inv.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//66	
	 prov = imread("binBastos10_Inv.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//67
	 prov = imread("binBastos11_Inv.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//68
	 prov = imread("binBastos12_Inv.jpg",CV_LOAD_IMAGE_GRAYSCALE);output.push_back(prov);	//69
	 
	 return output;
 }
 
 void printIDonImage(vector<int> ID,Mat image,vector<vector<Point> > contours){
	 /* Distinguishes the cards based on their ID number and prints its
	  * identification on the frame. */
	  int numCards = ID.size();
	  string number;
	  for (int i=0;i<numCards;i++){
		  // Determination of suit of the card. Draw its colored contour
		  if (ID[i] <16){drawContours(image,contours,i,Scalar(0,150,150),6,8);}
		  else if ((ID[i] > 15) && (ID[i] < 36)){drawContours(image,contours,i,Scalar(0,25,200),6,8);}
		  else if ((ID[i] > 35) && (ID[i] < 53)){drawContours(image,contours,i,Scalar(150,0,0),6,8);}
		  else{drawContours(image,contours,i,Scalar(50,200,75),6,8);}
		  // Determination of the number
		  if ((ID[i] == 0) || (ID[i] == 10)){number = "AS";}
		  else if(ID[i] == 1){number = "2";}
		  else if((ID[i] == 2) || (ID[i] == 11)){number = "3";}
		  else if(ID[i] == 3){number = "4";}
		  else if(ID[i] == 4){number = "5";}
		  else if(ID[i] == 5){number = "6";}
		  else if((ID[i] == 6) || (ID[i] == 12)){number = "7";}
		  else if((ID[i] == 7) || (ID[i] == 13)){number = "SOTA";}
		  else if((ID[i] == 8) || (ID[i] == 14)){number = "CABALLO";}
		  else if((ID[i] == 9) || (ID[i] == 15)){number = "REY";}
		  else if((ID[i] == 16) || (ID[i] == 26)){number = "AS";}
		  else if((ID[i] == 17) || (ID[i] == 27)){number = "2";}
		  else if((ID[i] == 18) || (ID[i] == 28)){number = "3";}
		  else if((ID[i] == 19) || (ID[i] == 29)){number = "4";}
		  else if((ID[i] == 20) || (ID[i] == 30)){number = "5";}
		  else if((ID[i] == 21) || (ID[i] == 31)){number = "6";}
		  else if((ID[i] == 22) || (ID[i] == 32)){number = "7";}
		  else if((ID[i] == 23) || (ID[i] == 33)){number = "SOTA";}
		  else if((ID[i] == 24) || (ID[i] == 34)){number = "CABALLO";}
		  else if((ID[i] == 25) || (ID[i] == 35)){number = "REY";}
		  else if((ID[i] == 36) || (ID[i] == 46)){number = "AS";}
		  else if(ID[i] == 37){number = "2";}
		  else if((ID[i] == 38) || (ID[i] == 47)){number = "3";}
		  else if(ID[i] == 39){number = "4";}
		  else if((ID[i] == 40) || (ID[i] == 48)){number = "5";}
		  else if(ID[i] == 41){number = "6";}
		  else if((ID[i] == 42) || (ID[i] == 49)){number = "7";}
		  else if((ID[i] == 43) || (ID[i] == 50)){number = "SOTA";}
		  else if((ID[i] == 44) || (ID[i] == 51)){number = "CABALLO";}
		  else if((ID[i] == 45) || (ID[i] == 52)){number = "REY";}
		  else if((ID[i] == 53) || (ID[i] == 63)){number = "AS";}
		  else if(ID[i] == 54){number = "2";}
		  else if((ID[i] == 55) || (ID[i] == 64)){number = "3";}
		  else if(ID[i] == 56){number = "4";}
		  else if((ID[i] == 57) || (ID[i] == 65)){number = "5";}
		  else if(ID[i] == 58){number = "6";}
		  else if((ID[i] == 59) || (ID[i] == 66)){number = "7";}
		  else if((ID[i] == 60) || (ID[i] == 67)){number = "SOTA";}
		  else if((ID[i] == 61) || (ID[i] == 68)){number = "CABALLO";}
		  else{number = "REY";}
		  RotatedRect rRect = minAreaRect(contours[i]);
		  Point2f pos = Point2f(rRect.center.x-50,rRect.center.y);
		  putText(image,number,pos,FONT_HERSHEY_DUPLEX,1,Scalar(0,0,0),2,8,false);
	  }
  }
 
 
