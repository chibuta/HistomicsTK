/*
C++ version of TraceBounds
*/
#include "trace_boundary_cpp.h"

#include <iostream>
#include <list>
#include <cmath>
#include <vector>
#include <set>
#include <algorithm>

struct Points {
    int x, y;
};

trace_boundary_cpp::trace_boundary_cpp()
{
}

void trace_boundary_cpp::rot90(int nrows, int ncols,
  std::vector <std::vector<int> > input,
  std::vector <std::vector<int> > &output)
{
    for (int i=0; i<nrows; i++){
      for (int j=0;j<ncols; j++){
        output[j][nrows-1-i] = input[i][j];
      }
    }
}

std::vector <std::vector<std::vector<int> > > trace_boundary_cpp::trace_boundary(std::vector <std::vector<int> > imLabels, int connectivity)
{
    std::vector <std::vector<std::vector<int> > > output;

    // start ----- rotate matrix

    int nrows_img = imLabels.size();
    int ncols_img = imLabels[0].size();

    // initialize matrix for 90, 180, 270 degrees
    std::vector <std::vector<int> > imLabels_90(ncols_img, std::vector<int>(nrows_img));
    std::vector <std::vector<int> > imLabels_180(nrows_img, std::vector<int>(ncols_img));
    std::vector <std::vector<int> > imLabels_270(ncols_img, std::vector<int>(nrows_img));

    // rotate label matrix for 90, 180, 270 degrees
    rot90(nrows_img, ncols_img, imLabels, imLabels_270);
    rot90(ncols_img, nrows_img, imLabels_270, imLabels_180);
    rot90(nrows_img, ncols_img, imLabels_180, imLabels_90);

    // end ----- rotate matrix


    // start ----- find unique id of labels

    // 2d to 1d array
    std::vector<int> im1D;
    im1D.reserve(nrows_img * ncols_img);
    for(int i = 0; i < nrows_img; i++) {
       const std::vector<int>& v = imLabels[i];
       im1D.insert(im1D.end(), v.begin(), v.end());
    }

    // initialize label_set with unique id for each label
    std::set<int> label_set( im1D.begin(), im1D.end() );

    // remove 0 from the set
    label_set.erase(0);

    // end ----- find unique id of labels


    // initialize iterator
    std::set<int>::iterator it;

    // loop from the second element of set
    for (it = label_set.begin(); it != label_set.end(); it++) {
        std::vector<Points> point;

        // put each point into struct
        for (int i=0; i < nrows_img; i++) {
            for (int j = 0; j < ncols_img; j++) {
                if (imLabels[i][j] == *it) {
                    point.push_back({i, j});
                }
            }
        }

        // get min max of x and y
        auto mmX = std::minmax_element(point.begin(), point.end(),
            [] (Points const& lhs, Points const& rhs) {return lhs.x < rhs.x;});
        auto mmY = std::minmax_element(point.begin(), point.end(),
            [] (Points const& lhs, Points const& rhs) {return lhs.y < rhs.y;});

        int minX = mmX.first->x;
        int maxX = mmX.second->x;
        int minY = mmY.first->y;
        int maxY = mmY.second->y;

        // initialize number of rows and cols of mask with padding
        int nrows = maxX-minX+3;
        int ncols = maxY-minY+3;

        std::vector <std::vector<int> > mask(nrows, std::vector<int>(ncols));
        std::vector <std::vector<int> > mask_90(ncols, std::vector<int>(nrows));
        std::vector <std::vector<int> > mask_180(nrows, std::vector<int>(ncols));
        std::vector <std::vector<int> > mask_270(ncols, std::vector<int>(nrows));

        for (int i = 1; i < nrows-1; i++) {
            for (int j = 1; j < ncols-1; j++) {
                mask[i][j] = imLabels[minX+i-1][minY+j-1];
                mask_90[j][i] = imLabels_90[ncols_img-maxY+j-2][minX+i-1];
                mask_180[i][j] = imLabels_180[nrows_img-maxX+i-2][ncols_img-maxY+j-2];
                mask_270[j][i] = imLabels_270[minY+j-1][nrows_img-maxX+i-2];
            }
        }

        // set current label number to 1
        for (int i = 1; i < nrows-1; i++) {
            for (int j = 1; j < ncols-1; j++) {
                if (mask[i][j] > 0) {
                  mask[i][j] = 1;
                }
                if (mask_180[i][j] > 0) {
                  mask_180[i][j] = 1;
                }
                if (mask_90[j][i] > 0) {
                  mask_90[j][i] = 1;
                }
                if (mask_270[j][i] > 0) {
                  mask_270[j][i] = 1;
                }
            }
        }

        // find starting x and y points
        int startX = 0;
        int startY = 0;
        bool flag = false;

        for(int i = 1; i < nrows-1; i++) {
          for(int j = 1; j < ncols-1; j++) {
             if((mask[i][j] > 0)&&(!flag)){
               // check if the nubmer of points is one
               if(!((mask[i][j+1] == 0)&&(mask[i+1][j] == 0)&&
                 (mask[i+1][j+1] == 0)&&(mask[i-1][j+1] == 0))) {
                   startX = j;
                   startY = i;
                   flag = true;
               }
             }
          }
        }

        // initialize infinity
        float inf = std::numeric_limits<float>::infinity();

        std::vector <std::vector<int> > coords;

        if (connectivity == 4) {
            coords = isbf(nrows, ncols, mask,
              mask_90, mask_180, mask_270, startX, startY, inf);
        }
        else {
            coords = moore(nrows, ncols, mask,
              mask_90, mask_180, mask_270, startX, startY, inf);
        }

        int ncols_coords = coords[0].size();
        // add window offset from original labels coordinates
        for(int i = 0; i < ncols_coords; i++) {
             coords[0][i] = coords[0][i] + minY - 1;
             coords[1][i] = coords[1][i] + minX - 1;
        }
        // append current coords to output vector
        output.push_back(coords);
    }

    return output;

}

std::vector <std::vector<int> > trace_boundary_cpp::moore(int nrows, int ncols,
  std::vector <std::vector<int> > mask, std::vector <std::vector<int> > mask_90,
  std::vector <std::vector<int> > mask_180, std::vector <std::vector<int> > mask_270,
  int startX, int startY, float inf)
{
    // initialize boundary vector
    std::list<int> boundary_listX;
    std::list<int> boundary_listY;
    // push the first x and y points
    boundary_listX.push_back(startX);
    boundary_listY.push_back(startY);

    // check degenerate case where mask contains 1 pixel
    int sum = 0;
    for(int i=0; i< nrows; i++){
        for(int j=0; j< ncols; j++){
            sum = sum + mask[i][j];
        }
    }

    // set size of X: the size of X is equal to the size of Y
    int sizeofX = 0;

    if (sum > 1) {

      // set defalut direction
      int DX = 1;
      int DY = 0;

      // define clockwise ordered indices
      int row[8] = {2, 1, 0, 0, 0, 1, 2, 2};
      int col[8] = {0, 0, 0, 1, 2, 2, 2, 1};
      int dX[8] = {-1, 0, 0, 1, 1, 0, 0, -1};
      int dY[8] = {0, -1, -1, 0, 0, 1, 1, 0};
      int oX[8] = {-1, -1, -1, 0, 1, 1, 1, 0};
      int oY[8] = {1, 0, -1, -1, -1, 0, 1, 1};

      // set the number of rows and cols for moore
      int rowMoore = 3;
      int colMoore = 3;

      float angle;

      // loop until true
      while(1) {

        std::vector <std::vector<int> > h(rowMoore, std::vector<int>(colMoore));

        // initialize a and b which are indices of ISBF
        int a = 0;
        int b = 0;
        int x = boundary_listX.back();
        int y = boundary_listY.back();


        if((DX == 1)&&(DY == 0)){
          for (int i = ncols-x-2; i < ncols-x+1; i++) {
            for (int j = y-1; j < y+2; j++) {
                h[a][b] = mask_90[i][j];
                b++;
            }
            b = 0;
            a++;
          }
          angle = M_PI/2;
        }

        else if((DX == 0)&&(DY == -1)){
          for (int i = y-1; i < y+2; i++) {
            for (int j = x-1; j < x+2; j++) {
                h[a][b] = mask[i][j];
                b++;
            }
            b = 0;
            a++;
          }
          angle = 0;
        }

        else if((DX == -1)&&(DY == 0)){
          for (int i = x-1; i < x+2; i++) {
            for (int j = nrows-y-2; j < nrows-y+1; j++) {
                h[a][b] = mask_270[i][j];
                b++;
            }
            b = 0;
            a++;
          }
          angle = 3*M_PI/2;
        }

        else{
          for (int i = nrows-y-2; i < nrows-y+1; i++) {
            for (int j = ncols-x-2; j < ncols-x+1; j++) {
                h[a][b] = mask_180[i][j];
                b++;
            }
            b = 0;
            a++;
          }
          angle = M_PI;
        }

        int Move = 0;
        bool is_moore = false;

        for (int i=0; i<8 ; i++){
          if(!is_moore){
            if (h[row[i]][col[i]] == 1){
                Move = i;
                is_moore = true;
            }
          }
        }

        int cX = oX[Move];
        int cY = oY[Move];
        DX = dX[Move];
        DY = dY[Move];

        // transform points by incoming directions and add to contours
        float s = sin(angle);
        float c = cos(angle);

        float p, q;

        p = c*cX - s*cY;
        q = s*cX + c*cY;

        x = boundary_listX.back();
        y = boundary_listY.back();

        boundary_listX.push_back(x+roundf(p));
        boundary_listY.push_back(y+roundf(q));

        float i, j;
        i = c*DX-s*DY;
        j = s*DX+c*DY;
        DX = roundf(i);
        DY = roundf(j);

        // get length of the current linked list
        sizeofX = boundary_listX.size();

        if (sizeofX > 3) {

          int fx1 = *boundary_listX.begin();
          int fx2 = *std::next(boundary_listX.begin(), 1);
          int fy1 = *boundary_listY.begin();
          int fy2 = *std::next(boundary_listY.begin(), 1);
          int lx1 = *std::prev(boundary_listX.end());
          int ly1 = *std::prev(boundary_listY.end());
          int lx2 = *std::prev(boundary_listX.end(), 2);
          int ly2 = *std::prev(boundary_listY.end(), 2);

          // check if the first and the last x and y are equal
          if ((sizeofX > inf)|| \
          ((lx1 == fx2)&&(lx2 == fx1)&&(ly1 == fy2)&&(ly2 == fy1))){
            // remove the last element
              boundary_listX.pop_back();
              boundary_listY.pop_back();
              break;
          }
        }
      }
    }

    std::vector <std::vector<int> > boundary(2, std::vector<int>(sizeofX));

    boundary[0].assign(boundary_listX.begin(), boundary_listX.end());
    boundary[1].assign(boundary_listY.begin(), boundary_listY.end());

    return boundary;
}


std::vector <std::vector<int> > trace_boundary_cpp::isbf(int nrows, int ncols,
  std::vector <std::vector<int> > mask, std::vector <std::vector<int> > mask_90,
  std::vector <std::vector<int> > mask_180, std::vector <std::vector<int> > mask_270,
  int startX, int startY, float inf)
{
    // initialize boundary vector
    std::list<int> boundary_listX;
    std::list<int> boundary_listY;

    // push the first x and y points
    boundary_listX.push_back(startX);
    boundary_listY.push_back(startY);

    // set defalut direction
    int DX = 1;
    int DY = 0;

    // set the number of rows and cols for ISBF
    int rowISBF = 3;
    int colISBF = 2;

    float angle;

    // set size of X: the size of X is equal to the size of Y
    int sizeofX;

    // loop until true
    while(1) {

      std::vector <std::vector<int> > h(rowISBF, std::vector<int>(colISBF));

      // initialize a and b which are indices of ISBF
      int a = 0;
      int b = 0;

      int x = boundary_listX.back();
      int y = boundary_listY.back();

      if((DX == 1)&&(DY == 0)){
        for (int i = ncols-x-2; i < ncols-x+1; i++) {
          for (int j = y-1; j < y+1; j++) {
              h[a][b] = mask_90[i][j];
              b++;
          }
          b = 0;
          a++;
        }
        angle = M_PI/2;
      }

      else if((DX == 0)&&(DY == -1)){
        for (int i = y-1; i < y+2; i++) {
          for (int j = x-1; j < x+1; j++) {
              h[a][b] = mask[i][j];
              b++;
          }
          b = 0;
          a++;
        }
        angle = 0;
      }

      else if((DX == -1)&&(DY == 0)){
        for (int i = x-1; i < x+2; i++) {
          for (int j = nrows-y-2; j < nrows-y; j++) {
              h[a][b] = mask_270[i][j];
              b++;
          }
          b = 0;
          a++;
        }
        angle = 3*M_PI/2;
      }

      else{
        for (int i = nrows-y-2; i < nrows-y+1; i++) {
          for (int j = ncols-x-2; j < ncols-x; j++) {
              h[a][b] = mask_180[i][j];
              b++;
          }
          b = 0;
          a++;
        }
        angle = M_PI;
      }

      // initialize cX and cY which indicate directions for each ISBF
      std::vector<int> cX(1);
      std::vector<int> cY(1);

      if (h[1][0] == 1) {
          // 'left' neighbor
          cX[0] = -1;
          cY[0] = 0;
          DX = -1;
          DY = 0;
      }
      else{
          if((h[2][0] == 1)&&(h[2][1] != 1)){
              // inner-outer corner at left-rear
              cX[0] = -1;
              cY[0] = 1;
              DX = 0;
              DY = 1;
          }
          else{
              if(h[0][0] == 1){
                  if(h[0][1] == 1){
                      // inner corner at front
                      cX[0] = 0;
                      cY[0] = -1;
                      cX.push_back(-1);
                      cY.push_back(0);
                      DX = 0;
                      DY = -1;
                  }
                  else{
                      // inner-outer corner at front-left
                      cX[0] = -1;
                      cY[0] = -1;
                      DX = 0;
                      DY = -1;
                  }
              }
              else if(h[0][1] == 1){
                // front neighbor
                cX[0] = 0;
                cY[0] = -1;
                DX = 1;
                DY = 0;
              }
              else{
                // outer corner
                DX = 0;
                DY = 1;
              }
          }
      }

      // transform points by incoming directions and add to contours
      float s = sin(angle);
      float c = cos(angle);

      if(!((cX[0]==0)&&(cY[0]==0))){
          for(int t=0; t< int(cX.size()); t++){
              float a, b;
              int cx = cX[t];
              int cy = cY[t];

              a = c*cx - s*cy;
              b = s*cx + c*cy;

              x = boundary_listX.back();
              y = boundary_listY.back();

              boundary_listX.push_back(x+roundf(a));
              boundary_listY.push_back(y+roundf(b));
          }
      }

      float i, j;
      i = c*DX - s*DY;
      j = s*DX + c*DY;
      DX = roundf(i);
      DY = roundf(j);
      // get length of the current linked list
      sizeofX = boundary_listX.size();

      if (sizeofX > 3) {

        int fx1 = *boundary_listX.begin();
        int fx2 = *std::next(boundary_listX.begin(), 1);
        int fy1 = *boundary_listY.begin();
        int fy2 = *std::next(boundary_listY.begin(), 1);
        int lx1 = *std::prev(boundary_listX.end());
        int ly1 = *std::prev(boundary_listY.end());
        int lx2 = *std::prev(boundary_listX.end(), 2);
        int ly2 = *std::prev(boundary_listY.end(), 2);
        int lx3 = *std::prev(boundary_listX.end(), 3);
        int ly3 = *std::prev(boundary_listY.end(), 3);

        // check if the first and the last x and y are equal
        if ((sizeofX > inf)|| \
        ((lx1 == fx2)&&(lx2 == fx1)&&(ly1 == fy2)&&(ly2 == fy1))){
          // remove the last element
            boundary_listX.pop_back();
            boundary_listY.pop_back();
            break;
        }
        if (int(cX.size()) == 2)
          if ((lx2 == fx2)&&(lx3 == fx1)&&(ly2 == fy2)&&(ly3 == fy1)){
            boundary_listX.pop_back();
            boundary_listY.pop_back();
            boundary_listX.pop_back();
            boundary_listY.pop_back();
            break;
        }
      }
    }

    std::vector <std::vector<int> > boundary(2, std::vector<int>(sizeofX));

    boundary[0].assign(boundary_listX.begin(), boundary_listX.end());
    boundary[1].assign(boundary_listY.begin(), boundary_listY.end());

    return boundary;
}

trace_boundary_cpp::~trace_boundary_cpp()
{
}
