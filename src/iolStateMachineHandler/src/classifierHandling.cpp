/* 
 * Copyright (C) 2011 Department of Robotics Brain and Cognitive Sciences - Istituto Italiano di Tecnologia
 * Author: Ugo Pattacini
 * email:  ugo.pattacini@iit.it
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * http://www.robotcub.org/icub/license/gpl.txt
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
*/

#include <sstream>
#include <vector>
#include <yarp/os/Log.h>

#include "classifierHandling.h"


/**********************************************************/
void Classifier::push(const Score &s)
{
    window.push_back(s);
    if (window.size()>winLen)
        window.pop_front();
}


/**********************************************************/
void Classifier::push(const bool isPositive)
{
    Score s;
    s.isPositive=isPositive;
    s.score=newScore;
    push(s);
}


// theta = arg min_theta(sum_i(misclassified(score_i(theta))))
/**********************************************************/
double Classifier::update()
{
    // debug: keep the current threshold
    return threshold;

    bool posExitCond=false;
    bool negExitCond=false;
    for (size_t i=0; i<window.size(); i++)
    {
        if (window[i].isPositive)
            posExitCond=true;
        else
            negExitCond=true;

        if (posExitCond && negExitCond)
            break;
    }

    double bestTheta=threshold;
    if (posExitCond && negExitCond)
    {
        size_t minPenalty=window.size()+1;

        for (size_t i=0; i<window.size(); i++)
        {
            double theta=window[i].score;
            size_t penalty=0;

            for (size_t j=0; j<window.size(); j++)
                if ((window[j].isPositive && (window[j].score<theta)) ||
                    (!window[j].isPositive && (window[j].score>theta)))
                    penalty++;

            if (penalty<=minPenalty)
            {
                bestTheta=theta;
                minPenalty=penalty;
            }
        }
    }

    return bestTheta;
}


/**********************************************************/
void Classifier::init(const double thres)
{
    name="";
    threshold=thres;
    winLen=10;
    window.clear();
}


/**********************************************************/
Classifier::Classifier(const double thres)
{
    init(thres);
}


/**********************************************************/
Classifier::Classifier(const Classifier &classifier)
{
    name=classifier.name;
    threshold=classifier.threshold;
    newScore=classifier.newScore;
    winLen=classifier.winLen;
    window=classifier.window;
}


/**********************************************************/
Classifier::Classifier(const string &name, const double thres)
{
    init(thres);
    this->name=name;
}


/**********************************************************/
Classifier::Classifier(const Bottle &options)
{
    fromBottle(options);
}


/**********************************************************/
bool Classifier::isThis(const double val) const
{
    return (val>=threshold);
}


/**********************************************************/
void Classifier::prepare(const double newScore)
{
    yInfo("Classifier %s: stored score %g",name.c_str(),newScore);
    this->newScore=newScore;
}


/**********************************************************/
void Classifier::declare(const bool isPositive)
{
    push(isPositive);
    yInfo("Classifier %s: score %g declared as %s",name.c_str(),
          newScore,isPositive?"positive":"negative");    

    double thresholdLatched=threshold;
    threshold=update();
    yInfo("Updating threshold of classifier %s: %g => (%g) => %g",
          name.c_str(),thresholdLatched,newScore,threshold);    
}


/**********************************************************/
void Classifier::negative()
{
    declare(false);
}


/**********************************************************/
void Classifier::positive()
{
    declare(true);
}


/**********************************************************/
void Classifier::fromBottle(const Bottle &options)
{
    init();

    if (options.check("name"))
        name=options.find("name").asString();

    if (options.check("threshold"))
        threshold=options.find("threshold").asFloat64();

    if (options.check("winLen"))
        winLen=options.find("winLen").asInt32();

    if (Bottle *item_list=options.find("items").asList())
    {
        for (int i=0; i<item_list->size()-1; i+=2)
        {
            Score s;
            s.isPositive=(item_list->get(i).asString()=="pos");
            s.score=item_list->get(i+1).asFloat64();
            push(s);
        }
    }
}


/**********************************************************/
Bottle Classifier::toBottle()
{
    Bottle options;

    // insert name
    Bottle &name_list=options.addList();
    name_list.addString("name");
    name_list.addString(name);

    // insert threshold
    Bottle &threshold_list=options.addList();
    threshold_list.addString("threshold");
    threshold_list.addFloat64(threshold);

    // insert winLen
    Bottle &winLen_list=options.addList();
    winLen_list.addString("winLen");
    winLen_list.addInt32(winLen);

    // insert window
    Bottle &window_list=options.addList();
    window_list.addString("items");
    Bottle &item_list=window_list.addList();
    for (size_t i=0; i<window.size(); i++)
    {
        item_list.addString(window[i].isPositive?"pos":"neg");
        item_list.addFloat64(window[i].score);
    }

    return options;
}


/**********************************************************/
ClassifiersDataBase::~ClassifiersDataBase()
{
    clear();
}


/**********************************************************/
void ClassifiersDataBase::clear()
{
    for (auto it=begin(); it!=end(); it++)
        if (it->second!=NULL)
            delete it->second;

    map<string,Classifier*>::clear();
}


/**********************************************************/
void ClassifiersDataBase::erase(iterator it)
{
    if (it->second!=NULL)
        delete it->second;

    map<string,Classifier*>::erase(it);
}


/**********************************************************/
int ClassifiersDataBase::processScores(Classifier *pClassifier,
                                       const Bottle &scores)
{
    int ret=-1;
    double maxScoreObj=0.0;

    for (int i=0; i<scores.size(); i++)
    {
        ostringstream tag;
        tag<<"blob_"<<i;

        double maxScoreNoObj=0.0;
        double scoreObj=0.0;

        Bottle *blobScores=scores.find(tag.str()).asList();
        if (blobScores==NULL)
            continue;

        for (int j=0; j<blobScores->size(); j++)
        {
            Bottle *item=blobScores->get(j).asList();
            if (item==NULL)
                continue;

            string name=item->get(0).asString();
            double score=item->get(1).asFloat64();

            if (name==pClassifier->getName())
            {
                if (pClassifier->isThis(score))
                    scoreObj=score;
            }
            else if (score>=maxScoreNoObj)
                maxScoreNoObj=score;
        }

        if ((scoreObj>maxScoreNoObj) && (scoreObj>maxScoreObj))
        {
            pClassifier->prepare(maxScoreObj=scoreObj);
            ret=i;
        }
    }

    return ret;
}


/**********************************************************/
string ClassifiersDataBase::findName(const Bottle &scores,
                                     const string &tag,
                                     double *score)
{
    string retName=OBJECT_UNKNOWN;
    if(score) *score = 0;
    Bottle *blobScores=scores.find(tag).asList();
    if (blobScores==NULL)
        return retName;

    // first find the most likely object for the given blob
    double maxScore=0.0; int imax=0;
    vector<double> s(blobScores->size(),-1.0);
    for (int i=0; i<blobScores->size(); i++)
    {
        Bottle *item=blobScores->get(i).asList();
        if (item==NULL)
            continue;

        string name=item->get(0).asString();
        double score=item->get(1).asFloat64();
        s[i]=score;

        auto it=find(name);
        if (it!=end())
        {
            if (it->second->isThis(score) && (score>maxScore))
            {
                maxScore=score;
                retName=name;
                imax=i;
            }
        }
    }

    // then double-check that the found object is a good prediction:
    // the remaining scores shall not overcome 3/4 of max score
    if (retName!=OBJECT_UNKNOWN)
    {
        for (size_t i=0; i<s.size(); i++)
        {
            if ((i!=imax) && (s[i]>0.75*maxScore))
                return OBJECT_UNKNOWN;
        }
    }

    if(score) *score = maxScore;
    return retName;
}




