# IoT Utility Monitoring & Analysis System

## Summary
I developed this project to better understand electricity usage patterns and identify possible cost-saving opportunities. The project combines **historical electricity data from the TNB portal** with **real-time readings from an ESP32-based sensor**, then presents both in a dashboard for monitoring and analysis.

I designed the overall requirements and logic for the system, especially from the data and reporting side. The firmware itself was written using **AI-generated C++ based on my specifications**, while I personally handled the **data cleaning, dashboard development, and analysis workflow**.

## Problem
Electricity usage is often reviewed only through monthly utility bills. While that shows the total amount consumed, it does not explain **when usage is highest**, **what usage patterns are driving the bill**, or **where savings opportunities may exist**.

Another challenge was the historical data itself. A large portion of the project relied on records exported from the **TNB portal**, and this data was not immediately ready for analysis. It needed cleaning, restructuring, and standardization before it could be used effectively in a dashboard.

This project was built to answer a practical question:

**How can raw and messy electricity data be turned into a clear monitoring tool that helps reveal usage trends and support cost-saving analysis?**

## Project Goal
The main goal of this project was to build a monitoring and analytics solution that makes electricity usage easier to understand over time.

More specifically, I wanted to:
- monitor electricity usage trends more clearly
- combine historical and live data into one reporting view
- identify peak usage periods and unusual patterns
- support analysis that could lead to cost-saving opportunities

## Data Sources

### 1. Historical TNB Data
A significant portion of the dataset came from the TNB portal. This historical data was useful for understanding long-term consumption trends, but it required substantial preparation before it could be analyzed properly.

I used **Power Query** to clean and transform the records by:
- fixing inconsistent formatting
- standardizing date and time fields
- preparing usage values for aggregation
- reshaping the dataset into a structure suitable for dashboard reporting

This step was important because the historical data formed the basis for long-term trend analysis.

### 2. Real-Time IoT Data
To complement the historical records, I also built a real-time monitoring layer using an **ESP32** with an optical sensor that reads pulse signals from the electricity meter.

These pulse readings were converted into:
- pulse count
- estimated kWh
- estimated power demand

The device sends these readings to a web endpoint, allowing them to be displayed in the dashboard as live monitoring data.

## My Contribution
My role focused mainly on the **data, reporting, and solution design** side of the project.

I was responsible for:
- defining the project requirements and monitoring objectives
- deciding what data the system needed to capture
- structuring the data for reporting and analysis
- cleaning and transforming historical TNB data using **Power Query**
- building the dashboard in **Looker Studio / Data Studio**
- designing visuals to make the usage trends easier to interpret

The firmware was produced using **AI-generated C++** based on the system logic and requirements I defined. I then focused on the dashboard and analytics layer to make the output useful from a reporting perspective.

## Dashboard and Analysis
The dashboard was designed to make both live and historical electricity data easier to understand.

It includes:
- a **latest reading** section for quick monitoring
- a **pulse count trend** chart for short-interval changes
- a **total usage KPI**
- an **average electricity usage by time of day** chart
- a **daily consumption trend** view
- a **usage vs peak temperature** comparison

One of the most useful views is the **Average Electricity Usage by Time of Day** chart, because it highlights when electricity demand tends to be higher or lower across the day.

The dashboard helps shift the focus from simply tracking total consumption to understanding **how usage behaves over time**.

## Outcome
The dashboard was able to surface clear electricity usage patterns from both the historical and live data.

From the **Average Electricity Usage by Time of Day** chart, usage appears to be:
- highest during the **early hours of the day**
- relatively lower and more stable through normal daytime hours
- increasing again in the **evening**

This suggests that electricity demand is not evenly distributed throughout the day, and that there are specific periods where consumption is consistently higher. These time-based patterns are useful because they help narrow down when energy-saving efforts should be focused.

From the **Daily Consumption and Peak Temperature Trends** view, daily electricity usage changes over time, while temperature remains fairly stable within a narrower range. This may suggest that temperature alone is not the only driver of energy use, and that operational behavior or equipment usage patterns are likely contributing to the fluctuations in consumption.

The dashboard also shows a **total recorded usage of 4,627.5 kWh**, giving a cumulative view of consumption over the tracked period, while the live monitoring section adds short-interval visibility into current usage behavior.

Overall, the system did more than just collect data — it made it possible to identify recurring usage trends, compare daily behavior, and spot periods that may offer opportunities for reducing unnecessary consumption.

## Conclusion
The main takeaway from this project is that electricity usage becomes much more actionable when it is viewed as a **pattern over time**, rather than just a monthly total.

Based on the dashboard, the data suggests that:
- there are identifiable peak periods where electricity usage is consistently higher
- some fluctuations in consumption are likely influenced by operational patterns rather than temperature alone
- live monitoring can complement historical data by providing immediate visibility into current behavior

This means the project can support more targeted follow-up analysis, such as identifying what activities or equipment are driving the early-day and evening usage peaks. In that sense, the project does not just monitor electricity — it creates a foundation for finding inefficiencies and exploring cost-saving opportunities through data.