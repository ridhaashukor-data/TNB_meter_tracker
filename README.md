# IoT Utility Monitoring & Analysis System

## Summary
This project was developed to monitor electricity usage trends of my house, more clearly and identify possible cost-saving opportunities. It combines **historical electricity data from the TNB portal** with **real-time readings from an ESP32-based sensor**, then presents both in a dashboard for analysis. Access the dashboard here --- https://datastudio.google.com/reporting/7b1ea349-50e3-4a9c-a05a-3335e09bc43c

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

- **Historical TNB portal data**, which was cleaned and transformed in Power Query and Excel before analysis. Date range, 1st Jan - 30th May 2026.
- **Real-time IoT data** collected from an ESP32 and optical pulse sensor attached to the electricity meter

The historical dataset was important for long-term trend analysis, while the live sensor readings added short-interval visibility into current electricity usage.

The prepared data was then visualized in **Looker Studio / Data Studio**.

## Findings

The dashboard revealed several clear patterns in household electricity usage.
<img width="725" height="544" alt="Screenshot 60% 2026-06-05 172455" src="https://github.com/user-attachments/assets/3f1383b2-9867-44bb-b9b3-98da74c794ca" />


First, the **Average Electricity Usage by Time of Day** chart shows that usage is consistently higher at night, while daytime usage is lower and more stable. Night-time usage is roughly **three times higher** than daytime usage.

This pattern suggests that **air conditioning is likely the main contributor to electricity consumption**. Although the water heater has the highest individual load in the house, it is only used briefly, so its overall effect is much smaller compared to AC, which runs for longer periods.

Second, **TOU (Time-of-Use)** pricing may not be the best fit for this household. TOU is a pricing structure where electricity costs more during peak hours and less during off-peak hours. In this case, usage increases strongly from **7pm onward**, which still falls within the **2pm–10pm peak-rate window**, meaning a significant portion of consumption would still be charged at the higher rate.

Third, the **Daily Consumption and Peak Temperature Trends** chart suggests that maximum temperature may explain higher usage on some days, but not consistently. This means temperature may have some influence, but it is probably not the main factor behind daily consumption changes. Additional variables would need to be tested to understand the pattern more fully.

Based on these findings, the most likely cost-saving opportunities are improving **air-conditioning efficiency**, such as upgrading to a more energy-efficient unit, and improving **attic airflow** to reduce indoor heat buildup.

## Conclusion

This project shows that combining historical utility data with real-time monitoring can give a much clearer picture of electricity usage than monthly bills alone. By turning the data into trend-based analysis, the dashboard helps narrow down which areas are most worth investigating for savings.

Overall, the project suggests that the strongest opportunities for reducing cost are likely to come from improving cooling efficiency rather than changing tariff structure alone. It also shows that more variables should be tested in future analysis to better understand what drives day-to-day changes in electricity use.
