#ifndef PATCHMATCHALGO_H
#define PATCHMATCHALGO_H

#include <QThread>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <vector>
#include "zone.h"

class PatchMatchAlgo : public QThread
{
    Q_OBJECT

public:
    bool canceled;

    PatchMatchAlgo(QImage *source, QImage *target, std::vector<Zone*> *zones, double xscale, double yscale, int patch_w, int patchmatch_iterations, int em_iterations);
    ~PatchMatchAlgo();
    void run();

private:
    float total_work;
    float work_done;
    int em_iterations;
    int patchmatch_iterations;
    int patch_w;
    int scale;
    QImage *source;
    QImage *target;
    int **annx, **anny, **annd;
    std::vector<Zone*> *zones;

    inline int distance(QImage *a, QImage *b, int ax, int ay, int bx, int by);
    inline void randomANN(QImage *source, QImage *target);
    inline void rescaleANN(QImage *source, QImage *target);
    inline void patchVoting(QImage *source, QImage *target);
    inline void patchMatch(QImage *source, QImage *target);
    inline void enforceFixedZone(QImage *source, QImage *target);
    inline bool isReplaceSatisfied(int sx, int sy, int tx, int ty);

signals:
    void progressed(double progress);
    void progressed(QImage progress);
    void finished();

public slots:
    void stop();
};

#endif
