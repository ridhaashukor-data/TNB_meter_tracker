# IoT Utility Monitoring & Analysis System

## Summary
This project was developed to monitor electricity usage trends more clearly and identify possible cost-saving opportunities. It combines **historical electricity data from the TNB portal** with **real-time readings from an ESP32-based sensor**, then presents both in a dashboard for analysis.

I designed the system requirements and reporting logic for the project. The firmware was written using **AI-generated C++ based on my specifications**, while I handled the **data cleaning, dashboard development, and analysis workflow**.

## Problem and Objective
Electricity usage is often reviewed only through monthly bills, which show the total cost but do not explain **when usage is highest**, **how consumption changes over time**, or **where savings opportunities might exist**.

At the same time, much of the historical data from the TNB portal was not analysis-ready and needed to be cleaned and restructured first.

The objective of this project was therefore to build a monitoring and reporting solution that could:
- combine historical and live electricity data
- reveal usage trends and peak periods
- support investigation into cost-saving opportunities

## Data and Approach
The project uses two main data sources:

- **Historical TNB portal data**, which was cleaned and transformed in **Power Query** before analysis
- **Real-time IoT data** collected from an ESP32 and optical pulse sensor attached to the electricity meter

The historical dataset was important for long-term trend analysis, while the live sensor readings added short-interval visibility into current electricity usage.

The prepared data was then visualized in **Looker Studio / Data Studio** through charts showing:
- latest reading
- pulse count trend
- total usage
- average electricity usage by time of day
- daily consumption trend
- daily consumption vs peak temperature

## Key Findings
The dashboard revealed several useful patterns in electricity usage.

First, the **Average Electricity Usage by Time of Day** chart shows that usage is generally:
- higher during the **early hours of the day**
- lower and more stable during most daytime hours
- rising again in the **evening**

This indicates that electricity demand is not evenly distributed throughout the day, and that certain periods consistently drive higher consumption.

Second, the dashboard shows a **total recorded usage of 4,627.5 kWh**, providing a cumulative picture of overall electricity consumption across the tracked period.

Third, the **Daily Consumption and Peak Temperature Trends** chart suggests that maximum temperature may explain higher usage in some cases, but not consistently. In most cases, temperature does not appear to have a strong influence on electricity usage, which means other variables are likely driving the fluctuations. More factors would need to be tested before drawing a stronger conclusion.

Another possible takeaway is that **TOU (Time-of-Use)** pricing may not be suitable in this case, since TOU is a tariff structure where electricity costs vary depending on the time of day, and the observed usage pattern still shows relatively higher demand during periods that may be harder to shift operationally.

## Implications
The findings suggest that the main value of the project is in showing electricity usage as a **pattern over time**, rather than only as a monthly total.

This makes it easier to:
- identify recurring peak periods
- investigate whether high-usage hours are operationally necessary
- explore whether some consumption can be reduced or shifted
- support follow-up analysis using additional variables beyond temperature

## Conclusion
This project shows how combining cleaned historical utility data with live IoT readings can create a more useful view of electricity usage. The dashboard does not just show how much electricity was used, but also when it was used and how that usage changes over time.

From the analysis, the most important insight is that electricity usage follows clear time-based patterns, while temperature alone does not fully explain the variation. This suggests that further analysis should focus on other operational variables to better understand the drivers of consumption and identify realistic cost-saving opportunities.